
## Compile and Run

1. Compile the code using the command `make -f Makefile`
2. You can run the code by `./memory_coordinator <interval>`

### Algorithm:

In each iteration:
1. Use `virDomainMemoryStats` to obtain each domain's available memory, unused memory, and actual ballon memory.
2. Release memories from domains that have unused memory more than the maximum unused threshold.
3. Give memories to domains that have unused memory less than the maximum used threshold.

### Note:
1. Release memories before give memories to allow giving more memory to designated domains.
2. When releasing or giving memories, only give 100mb each time.
3. If the host machine has less than 100mb, refuse to give more memories to guest machines.
4. If a guest machine is requiring more memory and will have more memory than its max limit, refuse to give this machine more memories.

