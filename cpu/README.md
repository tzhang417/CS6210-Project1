### Compile and Run

1. Compile the code using the command `make -f Makefile`
2. You can run the code by `./memory_coordinator <interval>`

### Algorithm
In each iteration
1. Use `virDomainPinVcpu` to pin guest machines to an assigned host machine and record the assignment.
2. Call custom function `getPercentage` to get each host machine's usage percentage.
3. Call custom function `balanced` to determine if the current percentage allocation is balanced.
4. If balanced, do nothing. If not balanced, call custom function `balance` to balance.

Custom function `getPercentage`
1. Use `virDomainGetCPUStats` to record each domain's cpu time.
2. Rest for the input interval.
3. Use `virDomainGetCPUStats` to get each domain's cpu time. 
4. Convert the difference of the cpu time after rest with the cpu time before rest into percentage.
4. Store the percentage in to each domain's respective host.

Custom function `balanced`
1. Get the most utilized and the least utilized host based on each host machine's usage percentage.
2. If the difference between usage percentage of the most utilized and the least utilized hosts is less than the threshold, the current scenario is balanced.
3. If the most utilized host has only 1 guest machine, the current scenario is balanced.
4. For other conditions, the current scenario is not balanced.

Custom function `balance`
1. Get the most utilized and the least utilized host based on each host machine's usage percentage.
2. Get the guest with the lowest usage percentage in the most utilized host.
3. Pin this guest to the least utilized host, while updating the assignment.
