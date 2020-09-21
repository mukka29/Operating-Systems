## Assignment #1 Unix System Calls and Library Functions:

The goal of this homework is to become familiar with the environment in hoare while practising system calls. I’ll like to see the use of perror and getopt in this submission.

Do Exercise 3.9: Process Fans (p. 88) in your text by Robbins/Robbins.

The exercise expands on the fan structure of Program 3.2 in the book through the development of a simple batch processing facility, called runsim. The program being developed in this exercise is a precursor to building a license manager.

Program 3.2 creates a fan of n processes by calling fork(2) in a loop. So, you will need to study that code well. The process fan is a vehicle to experiment with wait and with sharing of devices.

With the use of perror, I’ll like some meaningful error messages. The format for error messages should be:
proc_fan: Error: Detailed error message

where proc_fan is actually the name of the executable (argv[0]) and should be appropriately modified if the name of executable is changed without recompilation.

I’ll like the process to be executed by the following command line:
proc_fan -n 20

where -n is the option to indicate the max number of processes generated.

I’ll like some meaningful error messages. The format for error messages should be:
proc_fan: Error: Detailed error message

where proc_fan is actually the name of the executable (argv[0]) that you are trying to execute.

## How to Run this code:
1. In your command prompt type: make
2. This will generate the executable files, see below steps
3. Compile with make
* $ make runsim testsim
   * gcc -ggdb -Wall runsim.c -o runsim
   * gcc -ggdb -Wall testsim.c -o testsim

4. Execute
 * $ ./runsim 2 < fast.data
