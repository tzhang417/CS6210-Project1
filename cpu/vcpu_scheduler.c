#include <float.h>
#include <limits.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <libvirt/libvirt.h>

int main(int argc, char *argv[])
{
    virNodeInfo nodeInfo;
    virConnectPtr conn;
    virDomainPtr *domains;
    if (argc < 2)
    {
        printf("Invalid Command\n");
        return -1;
    }

    int interval = atoi(argv[1]);
    if (interval <= 0) 
    {
        printf("Invalid Interval\n");
        return -1;
    }

    conn = virConnectOpen("qemu:///system");
    if (conn == NULL)
    {
        printf("Failed to Connect to the Hypervisor");
    }

    virNodeGetInfo(conn, &nodeInfo);
    int pCpu = (int) nodeInfo.cpus;
    printf("Number of pCpus: %d\n", pCpu);

    int numDomains = virConnectListAllDomains(conn, &domains, VIR_CONNECT_LIST_DOMAINS_ACTIVE);
    int curP = 0;
    for (int i = 0; i < numDomains; i++)
    {
        curP %= pCpu;
        virDomainPinVcpu(domains[i], i, curP, pCpu);
        printf("vCpu %d Pinned to pCpu %d", i, curP);
        curP += 1;
    }
}