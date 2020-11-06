# Message Passing and Operating System Simulator

In this project you will be creating an empty shell of an OS simulator and doing some tasks in preparation for a
more comprehensive simulation later. This will require shared memory and some message passing.
## Operating System Simulator

This will be your main program and serve as the master process. You will start the operating system simulator (call the executable oss) as one main process who will fork multiple children and then fork off new children as some terminate.

oss will start by first allocating shared memory for a clock. The child processes and master will be able to view and modify this clock, protected by a critical section enforced by message queues. This shared memory clock should be made up of two integers. One integer to hold seconds, the other integer to hold nanoseconds. So if one integer has 5 and the other has 10000 then that would mean that the clock is showing 5 seconds and 10000 nanoseconds. This clock should start at 0.

In addition, there should be another shared memory integer, which will be used by child processes to indicate when they have terminated. Let us call this shared memory integer shmPID, which should be initialized to 0.

With the clock at zero, oss should then fork off the appropriate number of child processes as specified by our command line arguments. It should then loop over the critical section, incrementing the shared memory clock (with the amount of this increment based on a constant). Each iteration of this loop, it should examine shmPID. If it sees a number there, that means that the process with that PID has just terminated. At that point, it should log that the process has terminated at the time shown in our simulated clock and set shmPID back to zero.

After logging that a child has terminated, oss should then fork off another child and continue on with the loop. This process should continue until 2 seconds have passed in the simulated system, 100 processes in total have been generated or the executable has been running for the maximum time allotted. At that point oss should terminate
all children and then itself.

The increment per iteration of the critical section should be set high enough to cause turnover in your system. That is, processes terminating and being generated. If no processes are terminating before 2 seconds have passed, increase the increment until they do so.

The log file should look as follows:

<ul>Master: Child pid xxx is terminating at system clock time xx.xx</ul>
<ul>Master: Creating new child pid xxx at my time xx.xx</ul>
<ul>Master: Child pid xxx is terminating at system clock time xx.xx</ul>
<ul>Master: Creating new child pid xxx at my time xx.xx</ul>
...

## User Processes

The child processes of the oss are the user processes. These should be a separate executable from oss, run with exec from the fork of oss. These child processes will stick around for a random amount of simulated time and then terminate once our simulated clock has exceeded that time.

Th child processes should start by reading our simulated system clock. It should then generate a random duration number from 1 to some maximum number of nanosecond constant (A reasonable value might be something like 1000000). This represents how long this child should run.

It should then loop continually over a critical section of code. This critical section should be enforced through the user of message passing with msgsnd and msgrcv.

The child process should loop over the critical section, checking the system clock and determining if it has exceeded its lifetime. If it has, it should check shmPID. If it is set to 0, the child process should put its PID there and then terminate. If it is non-zero, it should continue looping over the critical section until it can indicate to master that it wants to terminate.

Your project should also have signal handling to terminate all processes if needed and clear up shared memory. In case of abnormal termination, make sure to remove any resources that are used. Make sure that your process automatically terminates itself with a timed interrupt.

Your main executable should use command line arguments. You must implement at least the following command line arguments using "getopt":

<ul>-h </ul>
<ul>-c x </ul>
<ul>-l filename </ul>
<ul>-t z </ul>

where x is the maximum number of child processes spawned (default 5) and filename is the log file used. The -t parameter determines the time in seconds when the master will terminate itself and all children (default 20).

## Compilation Steps:
Navigate to the directory "mukka_updated.3/Assignment3_version2" on hoare and issue the below commands
Compiling with make 
<ul>$ make</ul>
<ul>OR</ul>
<ul>$ gcc -Werror -ggdb oss.c -o oss</ul>
<ul>$ gcc -Werror -ggdb palin.c -o palin</ul>

## Execution: 
<ul> $ ./oss 2>users_log.txt </ul>
 
* After this program execution (main executable here are 'oss.c' and 'user.c' files), two  executable files are generated - namely "oss" and "users", along with the log files. The output here in this case gets into the logfiles one for master (log.txt) and one for user (users_log.txt) processes, master contains the output in the required format and I have used other one for user processes.

  
## Checking the result:
* Once the program is ececuted, required Executable files are generated. Along with them, log file also gets generated. 
* ls
* this command shows all the files present in the directory after the project is executed.
* 'cat log.txt' is used to view the contents of master log file and

## data.h File
* This file contains the constants used in the project.

## clean the executables:
*  make clean
<ul>or</ul>
* 	rm -f oss user
This command is used to remove oss and user executable files
* If log.txt and users_log.txt files also need to be removed, 'rm -f log.txt users_log.txt' is used

### Updated project info can always be found at
* [My Project](https://github.com/mukka29/Operating-Systems)
