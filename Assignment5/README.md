# Resource Management
In this part of the assignment, you will design and implement a resource management module for our Operating System Simulator oss. In this project, you will use the deadlock avoidance strategy, using maximum claims, to manage resources.

There is no scheduling in this project, but you will be using shared memory; so be cognizant of possible race conditions. Concurrency will be handled with message queues, with each user process checking the clock. Similarly to project 3 (and unlike project 4), in this project OSS will go into a loop and be incrementing the clock continually waiting for user processes to decide to do things. oss will need to check to see if it has received a message from user processes in this loop, but it must do a nonblocking receive to do this. The user processes themselves will always be using blocking receives. More details are given on this in later sections.

## Operating System Simulator
This will be your main program and serve as the master process. You will start the operating system simulator (call the executable oss) as one main process who will fork multiple children at random times. The randomness will be simulated by a logical clock that will be updated by oss as well as user processes. Thus, the logical clock resides in shared memory and is accessed as a critical resource using a semaphore. You should have two unsigned integers for the clock; one will show the time in seconds and the other will show the time in nanoseconds, offset from the beginning of a second.

In the beginning, oss will allocate shared memory for system data structures, including resource descriptors for each resource. All the resources are static but some of them may be shared. The resource descriptor is a fixed size structure and contains information on managing the resources within oss. Make sure that you allocate space to keep track of activities that affect the resources, such as request, allocation, and release. The resource descriptors will reside in shared memory and will be accessible to the child. Create descriptors for 20 resources, out of which about 20% should be sharable resources1. After creating the descriptors, make sure that they are populated with an initial number of resources; assign a number between 1 and 10 (inclusive) for the initial instances in each resource class. You may have to initialize another structure in the descriptor to indicate the allocation of specific instances of a resource to a process.

After the resources have been set up, fork a user process at random times (between 1 and 500 milliseconds of your logical clock). Make sure that you never have more than 18 user processes in the system. If you already have 18 processes, do not create any more until some process terminates. Your user processes execute concurrently and there is no scheduling performed. They run in a loop constantly till they have to terminate.

oss should decide whether the received requests for resources should be allocated to the processes or not. It should do this by running the deadlock avoidance algorithm to ensure that it does not allocate resources that could possibly result in a deadlock. If a process releases resources, it should update the resource tables, which could result in giving resources to some processes that are currently blocked. If it cannot allocate resources, the process that made that request should go in a queue waiting for the resource requested and it will then wait for a message from oss indicating that request was granted.

## User Processes

While the user processes are not actually doing anything, they will ask for resources at random times. You should have a parameter giving a bound B for the maximum time when a process should request (or let go of) a resource. Each process, when it starts, should generate a random number in the range [0, B] and after waiting that long, it should try and either request a new resource or release an already acquired resource. It should make that request by sending a message to oss using a message queue. It will then wait for a message back indicating the request is granted. At that point it will generate another B, register what the current system time is, and loop looking at the shared memory clock until B time interval has passed.

Note that the chance to request or release a resource should not be even. This percentage should be a constant in your system, but biased towards requesting more resources than it releases. If this is not the case, your processes will end up mostly using very few resources and we will never see a deadlock.

Since we are simulating deadlock avoidance, each user process starts with declaring its maximum claims. The claims can be generated using a random number generator, taking into account the fact that no process should ask for more than the maximum number of resources in the system. You will do that by generating a random number between 0 and the number of instances in the resource descriptor for the resource that has already been set up by oss.

The user processes can ask for resources at random times. Make sure that the process does not ask for more than the maximum number of available resource instances at any given time, the total for a process (request + allocation) should always be less than or equal to the maximum number of instances of a specified resources.

At random times (between 0 and 250ms), the process checks if it should terminate (based on some constant random chance). If so, it should deallocate all the resources allocated to it by communicating to oss that it is releasing all those resources. Make sure to do this only after a process has run for at least 1 second.

I want you to keep track of statistics during your runs. Keep track of how many requests have been granted of each type per process, as well as on average, how long are processes in a blocked state vs not.

Make sure that you have signal handling to terminate all processes, if needed. In case of abnormal termination, make sure to remove shared memory and semaphores.

When writing to the log file, you should have two ways of doing this. One setting (verbose on) should indicate in the log file every time oss gives someone a requested resource or when master sees that a user has finished with a resource. It should also log when a process is blocked on requesting a resource and when a process is unblocked when more resources become avaiable. In addition, periodically (maybe every 20 granted requests), output a table showing the current resources allocated to each process and which processes are blocked waiting on resources in the system.

An example of possible output might be:

OSS has detected Process P0 requesting R2 at time xxx:xxx
OSS granting P0 request R2 at time xxx:xxx
OSS has acknowledged Process P0 releasing R2 at time xxx:xxx
OSS has detected Process P1 requesting R3 at time xxx:xxx
OSS running deadlock avoidance at time xxx:xxx
  Safe state after granting request found
  OSS granting PP1 request R3 at time xxx:xxx
OSS has detected Process P4 requesting R1 at time xxx:xxx
OSS running deadlock avoidance at time xxx:xxx
  Processes P3, P4, P7 could deadlock in this scenario
  Unsafe state after granting request; request not granted
  P4 added to wait queue, waiting on R1
OSS has acknowledged that Process P2 is terminating
  Resources released: R1:1, R3:1, R4:5
Current System Resources:
  R0 R1 R2 R3 ...
P0 2 1 3 4 ...
P1 0 1 1 0 ...
P2 3 1 0 0 ...
P3 7 0 1 1 ...
P4 0 0 3 2 ...
P7 1 2 0 5 ...
...
%


## Compilation Steps:
Navigate to the directory "mukka.5/Assignment5" on hoare and issue the below commands
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


