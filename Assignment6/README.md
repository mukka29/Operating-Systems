# Memory Management
In this project we will be designing and implementing a memory management module for our Operating System Simulator oss.
In particular, we will be implementing the LRU (clock) page replacement algorithms. When a page-fault occurs, it will be necessary to swap in that page. If there are no empty frames, your algorithm will select the victim frame based on our LRU replacement policy.
Each frame should also have an additional dirty bit, which is set on writing to the frame. This bit is necessary to consider dirty bit optimization when determining how much time these operations take. The dirty bit will be implemented as a part of the page table

## Operating System Simulator
This will be your main program and serve as the master process. You will start the operating system simulator (call the executable oss) as one main process who will fork multiple children at random times. The randomness will be simulated by a logical clock that will be updated by oss as well as user processes. Thus, the logical clock resides in shared memory. You should have two unsigned integers for the clock; one will show the time in seconds and the other will show the time in nanoseconds, offset from the beginning of a second.
In the beginning, oss will allocate shared memory for system data structures, including page table. You can create fixed sized arrays for page tables, assuming that each process will have a requirement of less than 32K memory, with each page being 1K. The page table should also have a delimiter indicating its size so that your programs do not access memory beyond the page table limit. The page table should have all the required fields that may be implemented by bits or character data types.

Assume that your system has a total memory of 256K. You will require a frame table, with data required such as reference byte and dirty bit. Use a bit vector to keep track of the unallocated frames. After the resources have been set up, fork a user process at random times (between 1 and 500 milliseconds of your logical clock). Make sure that you never have more than 18 user processes in the system. If you already have 18 processes, do not create any more until some process terminates. 18 processes is a hard limit and you should implement it using a #define macro. Thus, if a user specifies an actual number of processes as 30, your hard limit will still limit it to no more than 18 processes at any time in the system. Your user processes execute concurrently and there is no scheduling performed. They run in a loop constantly till they have to terminate.
oss will monitor all memory references from user processes and if the reference results in a page fault, the process will be suspended till the page has been brought in. It is up to you how you do synchronization for this project, so for example you could use message queues or a semaphore for each process. Effectively, if there is no page fault, oss just increments the clock by 10 nanoseconds and sends a signal on the corresponding semaphore. In case of page fault, oss queues the request to the device. Each request for disk read/write takes about 14ms to be fulfilled. In case of page fault, the request is queued for the device and the process is suspended as no signal is sent on the semaphore. The request at the head of the queue is fulfilled once the clock has advanced by disk read/write time since the time the request was found at the head of the queue. The fulfillment of request is indicated by showing the page in memory in the page table. oss should periodically check if all the processes are queued for device and if so, advance the clock to fulfill the request at the head. We need to do this to resolve any possible deadlock in case memory is low and all processes end up waiting.
While a page is referenced, oss performs other tasks on the page table as well such as updating the page reference, setting up dirty bit, checking if the memory reference is valid and whether the process has appropriate permissions on the frame, and so on.

When a process terminates, oss should log its termination in the log file and also indicate its effective memory access time. oss should also print its memory map every second showing the allocation of frames. You can display unallocated frames by a period and allocated frame by a +.

For example at least something like..

Master: P2 requesting read of address 25237 at time xxx:xxx
Master: Address 25237 in frame 13, giving data to P2 at time xxx:xxx
Master: P5 requesting write of address 12345 at time xxx:xxx
Master: Address 12345 in frame 203, writing data to frame at time xxx:xxx
Master: P2 requesting write of address 03456 at time xxx:xxx
Master: Address 12345 is not in a frame, pagefault
Master: Clearing frame 107 and swapping in p2 page 3
Master: Dirty bit of frame 107 set, adding additional time to the clock
Master: Indicating to P2 that write has happened to address 03456
Current memory layout at time xxx:xxx is:
      Occupied DirtyBit timeStamp
Frame 0:  No      0     xxxxx
Frame 1:  Yes     1     xxxxx
Frame 2:  Yes     0     xxxxx
Frame 3:  Yes     1     xxxxx
where Occupied indicates if we have a page in that frame, the timeStamp is used for the LRU replacement policy and the dirty bit indicates if the frame has been written to.

## User Processes

Each user process generates memory references to one of its locations. This should be done in two different ways. In particular, your project should take a command line argument (-m x) where x specifies which of the following memory request schemes the child processes :

* The first memory request scheme is simple, when a process needs to generate an address to request, it simply
generates a random value from 0 to the limit of the process memory (32k).

* The second memory request scheme tries to favor certain pages over others. You should implement a scheme where each of the 32 pages of a processes memory space has a different weight of being selected. In particular, the weight of a processes page n should be 1/n. So the first page of a process would have a weighting of 1, the second page a weighting of 0.5, the third page a weighting of 0.3333 and so on. Your code then needs to select one of those pages based on these weightings. This can be done by storing those values in an array and then, starting from the beginning, adding to each index of the array the value in the preceding index. For example, if our weights started off as [1,0.5,0.3333,0.25,...] then after doing this process we would get an array of [1,1.5,1.8333,2.0833,...]. Then you generate a random number from 0 to the last value in the array and then travel down the array until you find a value greater than that value and that index is the page you should request.

Now you have the page of the request, but you need the offset still. Multiply that page number by 1024 and then add a random offset of from 0 to 1023 to get the actual memory address requested. Note that we are only simulating this and actually do not have anything to read or write.

Once this is done, you now have a memory address, but we still must determine if it is a read or write. Do this with randomness, but bias it towards reads. This information (the address requested and whether it is a read or write) should be conveyed to oss. The user process will wait on its semaphore (or message queue if implemented that way) that will be signaled by oss. oss checks the page reference by extracting the page number from the address, increments the clock as specified above, and sends a signal on the semaphore if the page is valid.

At random times, say every 1000 ± 100 memory references, the user process will check whether it should terminate. If so, all its memory should be returned to oss and oss should be informed of its termination.

The statistics of interest are:
* Number of memory accesses per second
* Number of page faults per memory access
* Average memory access speed

You should terminate after more than 100 processes have gotten into your system, or if more than 2 real life seconds have passed. Make sure that you have signal handling to terminate all processes, if needed. In case of abnormal termination, make sure to remove shared memory, queues and semaphores.

In your README file, discuss the performance of the page replacement algorithm on both of the different page request schemes. Which one is a more realistic model of the way pages would actually be requested?


## Compilation Steps:
Navigate to the directory "mukka.6/Assignment6" on hoare and issue the below commands
* Compiling with make 
<ul>$ make</ul>
<ul>OR</ul>
* <ul>$ gcc -Werror -ggdb -Wall -c queue.c </ul>
  <ul>$ gcc -Werror -ggdb -Wall -c res.c </ul>
  <ul>$ gcc -Werror -ggdb -Wall oss.c queue.o res.o -o oss </ul>
  <ul>$ gcc -Werror -ggdb -Wall user.c res.o -o user </ul>

## Execution: 
<ul> "$ ./oss" </ul>
<ul> OR </ul>
<ul> "$ ./oss -v" (Verbose on)</ul>
* With the verbose mode, it gives statistics
 
* After this program execution, two executable files are generated - namely "oss" and "user", along with the log file. This log file contains the output in the required format.

## Checking the result:
* Once the program is ececuted and ran, required Executable files are generated. Along with them, above mentioned log file also get generated. 
* $ ls
* this command shows all the files present in the directory after the project is executed.
* '$ cat log.txt' command is used to view the contents of log file

Note: The log.txt file (generated with verbose mode, -v) is attached in the files section as a way to show how it exactly prints the output.

## data.h, res.h and queue.h Files
* These files contains the constants, variables and functions used in the other files .

## clean the executables:
<ul> make clean </ul>
<ul> or </ul>
<ul> rm -f oss user </ul>
This command is used to remove oss and user executable files. If log file and other generated files needs to be removed the same 'rm' command with file names can be used.

### Updated project info can always be found at
* [My Project](https://github.com/mukka29/Operating-Systems)



