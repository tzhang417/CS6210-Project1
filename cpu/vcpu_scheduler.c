#include<stdio.h>
#include<stdlib.h>
#include<libvirt/libvirt.h>
#include<math.h>
#include<string.h>
#include<unistd.h>
#include<limits.h>
#include<signal.h>
#define MIN(a,b) ((a)<(b)?a:b)
#define MAX(a,b) ((a)>(b)?a:b)

int is_exit = 0; // DO NOT MODIFY THIS VARIABLE


void CPUScheduler(virConnectPtr conn,int interval);

/*
DO NOT CHANGE THE FOLLOWING FUNCTION
*/
void signal_callback_handler()
{
	printf("Caught Signal");
	is_exit = 1;
}

/*
DO NOT CHANGE THE FOLLOWING FUNCTION
*/
int main(int argc, char *argv[])
{
	virConnectPtr conn;

	if(argc != 2)
	{
		printf("Incorrect number of arguments\n");
		return 0;
	}

	// Gets the interval passes as a command line argument and sets it as the STATS_PERIOD for collection of balloon memory statistics of the domains
	int interval = atoi(argv[1]);
	
	conn = virConnectOpen("qemu:///system");
	if(conn == NULL)
	{
		fprintf(stderr, "Failed to open connection\n");
		return 1;
	}

	// Get the total number of pCpus in the host
	signal(SIGINT, signal_callback_handler);

	while(!is_exit)
	// Run the CpuScheduler function that checks the CPU Usage and sets the pin at an interval of "interval" seconds
	{
		CPUScheduler(conn, interval);
		sleep(interval);
	}

	// Closing the connection
	virConnectClose(conn);
	return 0;
}

void CPUScheduler(virConnectPtr conn, int interval)
{
    virNodeInfo nodeInfo;
    virDomainPtr *domains;

    virNodeGetInfo(conn, &nodeInfo);
    int pCpu = (int) nodeInfo.cpus;
    printf("Number of pCpus: %d\n", pCpu);

    int numDomains = virConnectListAllDomains(conn, &domains, VIR_CONNECT_LIST_DOMAINS_ACTIVE);
    printf("Number of domains: %d\n", numDomains);
    unsigned char cpuMap = 0x01;
    int *domainToCpu = calloc(numDomains, sizeof(int));
    for (int i = 0; i < numDomains; i++)
    {
        virDomainPinVcpu(domains[i], 0, &cpuMap, VIR_CPU_MAPLEN(nodeInfo.cpus));
        domainToCpu[i] = i % pCpu;
        printf("vCpu %d Pinned to pCpu %d\n", i, cpuMap);
        if ((cpuMap << 1) >= (1 << pCpu))
        {
            cpuMap = 0x01;
        }
        else
        {
            cpuMap <<= 1;
        }
    }

    double *cpuPercentage = calloc(pCpu, sizeof(double));
    double *domainPercentage = calloc(numDomains, sizeof(double));
    while(1)
    {
        getPercentage(domains, numDomains, cpuPercentage, domainToCpu, pCpu, interval, domainPercentage);
        for (int i = 0; i < pCpu; i++)
        {
            printf("CPU %d's usage is %f\n", i, cpuPercentage[i]);
        }
        if (balanced(cpuPercentage, pCpu, domainToCpu, numDomains))
        {
            printf("Balanced!\n");
        }
        else
        {
            printf("Not Balanced\n");
            balance(cpuPercentage, pCpu, domainToCpu, numDomains, nodeInfo, domains, domainPercentage);
        }
        sleep(interval);     
    }

}

void getPercentage(virDomainPtr *domains, int numDomains, double *cpuPercentage, int *domainToCpu, int pCpu, int interval, double *domainPercentage)
{
    unsigned long long prevCpuTime[numDomains]; 
    memset(cpuPercentage, 0, pCpu * sizeof(double));
    memset(domainPercentage, 0, numDomains * sizeof(double));
    int nparams = 1;
    virTypedParameterPtr params = calloc(1, sizeof(virTypedParameter));
    for (int i = 0; i < numDomains; i++)
    {
        virDomainGetCPUStats(domains[i], params, nparams, -1, 1, 0);
        prevCpuTime[i] = params[0].value.l;
    }
    
    sleep(interval);
    
    for (int i = 0; i < numDomains; i++)
    {
        virDomainGetCPUStats(domains[i], params, nparams, -1, 1, 0);
        cpuPercentage[domainToCpu[i]] += 100.0 * (params[0].value.l - prevCpuTime[i]) / (interval * 1000000000);
        domainPercentage[i] = 100.0 * (params[0].value.l - prevCpuTime[i]) / (interval * 1000000000);
    }
}

int balanced(double *cpuPercentage, int pCpu, int *domainToCpu, int numDomains)
{
    int maxCpu = 0;
    int minCpu = 0;
    double threshold = 10.0;
    for (int i = 1; i < pCpu; i++)
    {
        if (cpuPercentage[i] > cpuPercentage[maxCpu])
        {
            maxCpu = i;
        }
        if (cpuPercentage[i] < cpuPercentage[minCpu])
        {
            minCpu = i;
        }
    }
    if (cpuPercentage[maxCpu] - cpuPercentage[minCpu] < threshold)
    {
        return 1;
    } 
    else
    {
        int count = 0;
        for (int i = 0; i < numDomains; i++)
        {
            if (domainToCpu[i] == maxCpu)
            {
                count++;
            }
        }
        if (count == 1)
        {
            return 1;
        }
        return 0;
    }
}

void balance(double *cpuPercentage, int pCpu, int *domainToCpu, int numDomains, virNodeInfo nodeInfo, virDomainPtr *domains, double *domainPercentage)
{
    int maxCpu = 0;
    int minCpu = 0;
    for (int i = 1; i < pCpu; i++)
    {
        if (cpuPercentage[i] > cpuPercentage[maxCpu])
        {
            maxCpu = i;
        }
        if (cpuPercentage[i] < cpuPercentage[minCpu])
        {
            minCpu = i;
        }
    }

    int domainToMove = -1;
    unsigned long long domainToMoveTime = -1;
    for (int i = 0; i < numDomains; i++)
    {
        if (domainToCpu[i] == maxCpu)
        {
            if (domainToMove == -1)
            {
                domainToMove = i;
                domainToMoveTime = domainPercentage[i];
            }
            else if (domainPercentage[i] < domainToMoveTime)
            {
                domainToMove = i;
                domainToMoveTime = domainPercentage[i];
            }
            
        }
    }
    domainToCpu[domainToMove] = minCpu;
    unsigned char cpuMap = 1 << minCpu;
    virDomainPinVcpu(domains[domainToMove], 0, &cpuMap, VIR_CPU_MAPLEN(nodeInfo.cpus));
    printf("vCpu %d Pinned to pCpu %d\n", domainToMove, cpuMap);
}