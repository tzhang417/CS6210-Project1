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

int is_exit = 0; // DO NOT MODIFY THE VARIABLE

void MemoryScheduler(virConnectPtr conn,int interval);

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

	signal(SIGINT, signal_callback_handler);

	while(!is_exit)
	{
		// Calls the MemoryScheduler function after every 'interval' seconds
		MemoryScheduler(conn, interval);
		sleep(interval);
	}

	// Close the connection
	virConnectClose(conn);
	return 0;
}

/*
COMPLETE THE IMPLEMENTATION
*/
void MemoryScheduler(virConnectPtr conn, int interval)
{
	virNodeInfo nodeInfo;
    virDomainPtr *domains;
	int low = 100;
	int high = 300;

    virNodeGetInfo(conn, &nodeInfo);
    int pCpu = (int) nodeInfo.cpus;
    printf("Number of pCpus: %d\n", pCpu);

    int numDomains = virConnectListAllDomains(conn, &domains, VIR_CONNECT_LIST_DOMAINS_ACTIVE);
    printf("Number of domains: %d\n", numDomains);

	unsigned long long* unusedMem = calloc(numDomains, sizeof(unsigned long long));
	unsigned long long* availableMem = calloc(numDomains, sizeof(unsigned long long));
	unsigned long long* balloonMem = calloc(numDomains, sizeof(unsigned long long));

	virDomainMemoryStatStruct stats = calloc(VIR_DOMAIN_MEMORY_STAT_NR, sizeof(virDomainMemoryStatStruct));;
	for (int i = 0; i < numDomains; i++)
	{
		int statsLength = virDomainMemoryStats(domains[i], stats, VIR_DOMAIN_MEMORY_STAT_NR, 0);
		for (int j = 0; j < statsLength; j++)
		{
			virDomainMemoryStatStruct stat = stats[j];
			if (stat.tag == VIR_DOMAIN_MEMORY_STAT_UNUSED)
			{
				unusedMem[i] = stat.val / 1024;
			}
			else if (stat.tag == VIR_DOMAIN_MEMORY_STAT_AVAILABLE)
			{
				availableMem[i] = stat.val / 1024;
			}
			else if (stat.tag == VIR_DOMAIN_MEMORY_STAT_ACTUAL_BALLOON)
			{
				balloonMem[i] = stat.val / 1024;
			}
		}
		printf("Domain %d: unused %lld; available %lld; actual balloon %lld\n", i, unusedMem[i], availableMem[i], balloonMem[i]);
	}

	for (int i = 0; i < numDomains; i++)
	{
		if (unusedMem[i] > high * 1024)
		{
			virDomainSetMemory(domains[i], (balloonMem[i] - availableMem[i] + (low + high) / 2 ) * 1024);
		}
	}

	for (int i = 0; i < numDomains; i++)
	{
		if (unusedMem[i] < low * 1024)
		{
			virDomainSetMemory(domains[i], (balloonMem[i] - availableMem[i] + (low + high) / 2 ) * 1024);
		}
	}
}

