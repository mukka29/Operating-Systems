# Process Scheduling
The goal of this homework is to learn about process scheduling inside an operating system. You will work on the specified scheduling
algorithm and simulate its performance

## Task
In this project, you will simulate the process scheduling part of an operating system. You will implement time-based scheduling, ignoring almost every other aspect of the OS. Please use message queues for synchronization

## Operating System Simulator

The operating system simulator, or OSS, will be your main program and serve as the master process. You will start the simulator (call the executable oss) as one main process who will fork multiple children at random times. The randomness will be simulated by a logical clock that will also be updated by oss.

In the beginning, oss will allocate shared memory for system data structures, including a process table with a process control block for each user process. The process control block is a fixed size structure and contains information to manage the child process scheduling. Notice that since it is a simulator, you will not need to allocate space to save the context of child processes. But you must allocate space for scheduling-related items such as total CPU time used, total time in the system, time used during the last burst, your local simulated pid, and process priority, if any. The process control block resides in shared memory and is accessible to the children. Since we are limiting ourselves to 20 processes in this class, you should allocate space for up to 18 process control blocks. Also create a bit vector, local to oss, that will help you keep track of the process control blocks (or process IDs) that are already taken.

oss simulates the passing of time by using a simulated system clock. The clock is stored in two shared integers in memory, one which stores seconds and the other nanoseconds. so use two unsigned integers for the clock. oss will be creating user processes at random intervals, say every second on an average. While the simulated clock (the two integers) is viewable by the child processes, it should only be advanced (changed) by oss.

The way the clock is implemented is different from the previous project. oss simulates time passing in the system by adding time to the clock and as it is the only process that would change the clock, if a user process uses some time, oss should indicate this by advancing the clock. In the same fashion, if oss does something that should take some time if it was a real operating system, it should increment the clock by a small amount to indicate the time it spent.

oss will create user processes at random intervals (of simulated time), so you will have two constants; let us call them maxTimeBetweenNewProcsNS and maxTimeBetweenNewProcsSecs. oss will launch a new user process based on a random time interval from 0 to those constants. It generates a new process by allocating and initializing the process control block for the
process and then, forks the process. The child process will execl the binary. I would suggest setting these constants initially to spawning a new process about every 1 second, but you can experiment with this later to keep the system busy.

oss will run concurrently with all the user processes. After it sets up all the data structures, it enters a loop where it generates and schedules processes. It generates a new process by allocating and initializing the process control block for the process and then, forks the process. The child process will execl the binary.

oss will be in control of all concurrency. In the beginning, there will be no processes in the system but it will have a time in the future where it will launch a process. If there are no processes currently ready to run in the system, it should increment the clock until it is the time where it should launch a process. It should then set up that process, generate a new time where it will launch a process and then using a message queue, schedule a process to run by sending it a message. It should then wait for a message back from that process that it has finished its task. If your process table is already full when you go to generate a process, just skip that generation, but do determine another time in the future to try and generate a new process.

Advance the logical clock by 1.xx seconds in each iteration of the loop where xx is the number of nanoseconds. xx will be a random number in the interval [0,1000] to simulate some overhead activity for each iteration. A new process should be generated every 1 second, on average. So, you should generate a random number between 0 and 2 assigning it to time to create new process. If your clock has passed this time since the creation of last process, generate a new process (and execl it).

oss acts as the scheduler and so will schedule a process by sending it a message using a message queue. When initially started, there will be no processes in the system but it will have a time in the future where it will launch a process. If there are no processes currently ready to run in the system, it should increment the clock until it is the time where it should launch a process. It should then set up that process, generate a new time where it will create a new process and then using a message queue, schedule a process to run by sending it a message. It should then wait for a message back from that process that it has finished its task. If your process table is already full when you go to generate a process, just skip that generation, but do determine another time in the future to try and generate a new process.

## User Processes

All user processes are alike but simulate the system by performing some tasks at random times. The user process will keep checking in the shared memory location if it has been scheduled and once scheduled, it will start to run. It should generate a random number to check whether it will use the entire quantum, or only a part of it (a binary random number will be sufficient for this purpose). If it has to use only a part of the quantum, it will generate a random number in the range [0,quantum] to see how long it runs. 

The user processes will wait on receiving a message giving them a time slice and then it will simulate running. They do not do any actual work but instead send a message to oss saying how much time they used and if they had to use I/O or had to terminate.

As a constant in your system, you should have a probability that a process will terminate when scheduled. I would suggest this probability be fairly small to start. Processes will then, using a random number, use this to determine if they should terminate. Note that when creating this random number you must be careful that the seed you use is different for all processes, so I suggest seeding off of some function of a processes pid. If it would terminate, it would of course use some random amount of its timeslice before terminating. It should indicate to oss that it has decided to terminate and also how much of its timeslice it used, so that oss can increment the clock by the appropriate amount.


Once it has decided that it will not terminate, then we have to determine if it will use its entire timeslice or if it will get blocked on an event. This should be determined by a random number between 0 and 1. If it uses up its timeslice, this information should be conveyed to master. Otherwise, the process starts to wait for an event that will last for r.s seconds where r and s are random numbers with range [0, 3] and [0, 1000] respectively, and 3 indicates that the process gets preempted after using p% of its assigned quantum, where p is a random number in the range [1, 99]. As this could happen for multiple processes, this will require a blocked queue, checked by oss every time it makes a decision on scheduling to see if it should wake up these processes and put them back in the appropriate queues. Note that the simulated work of moving a process from a blocked queue to a ready queue would take more time than a normal scheduling decision so it would make sense to increment the system clock to indicate this.

Your simulation should end with a report on average wait time, average CPU utilization and average time a process waited in a blocked queue. Also include how long the CPU was idle with no ready processes. Make sure that you have signal handing to terminate all processes, if needed. In case of abnormal termination, make sure to remove shared memory and message queues

## Compilation Steps:
Navigate to the directory "mukka.4/Assignment4" on hoare and issue the below commands
* Compiling with make 
<ul>$ make</ul>
<ul>OR</ul>

* <ul>$ gcc -Werror -ggdb -c queue.c </ul>
  <ul>$ gcc -Werror -ggdb oss.c queue.o -o oss </ul>
  <ul>$ gcc -Werror -ggdb user.c -o user </ul>

## Execution: 
<ul> $ ./oss </ul>
 
* After this program execution (main executable here are 'oss.c' and 'user.c' files), two  executable files are generated - namely "oss" and "users", along with the log file. This log file contains the output in the required format.


## Checking the result:
* Once the program is ececuted and run, required Executable files are generated. Along with them, above mentioned log file also get generated. 
* $ ls
* this command shows all the files present in the directory after the project is executed.
* '$ cat log.txt' command is used to view the contents of log file

## data.h and queue.h Files
* These files contains the constants used in the project.

## clean the executables:
<ul> make clean </ul>
<ul> or </ul>
<ul> rm -f oss user </ul>
This command is used to remove oss and user executable files. If log file and other generated files needs to be removed the same 'rm' command with file names can be used.

### Updated project info can always be found at
* [My Project](https://github.com/mukka29/Operating-Systems)

