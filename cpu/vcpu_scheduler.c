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
    for (int i = 0; i < numDomains; i++)
    {
        virDomainPinVcpu(domains[i], 0, &cpuMap, VIR_CPU_MAPLEN(nodeInfo.cpus));
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

    getPercentage(domains, numDomains, interval);


    virConnectClose(conn);
}

void getPercentage(virDomainPtr *domains, int numDomains, int interval)
{
    int nparams = 1;
    for (int i = 0; i < numDomains; i++)
    {
        virTypedParameterPtr params = calloc(1, sizeof(virTypedParameter));
        virDomainGetCPUStats(domains[i], params, nparams, -1 ,1, 0);
        printf("vCpu %d uses %lld cpu time\n", i, params[0].value.l);
        free(params);
    }
}