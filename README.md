# Virtual-Memory-Simulator

To run a sample program navigate to src directory and run: 
./main ../plist.txt ../ptrace.txt 16 LRU +

This will run the main executable with the following arguments:
../plist.txt: the process list file to be read
../ptrace.txt: the process trace file to be read
16: Page size (Use 2, 4, 8, 16, or 32)
LRU: Page Replacement aglortithm (Use: LRU, Clock, or FIFO)
+: Pre-paging vs demand paging (- is demand paging, + is pre-paging)

Virtual memory simulator capable of simulating process traces and properly page swapping in the following three algorithms:
LRU, Clock, FIFO

Note: LRU's time keep is done using a cycle counter.
