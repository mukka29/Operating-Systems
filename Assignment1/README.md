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

## Implementation
* To implement this Assignment I have created files named: runsim.c and testsim.c along with these I have some files named: fast.data and testing.data
* Here, runsim is used as proc_fan/Name of the executable which fans-out/executes other things(files with .data in this case contain other programs that are to be fan out by runsim) and these data files are used for piping in to the CLI while executing the code.
* Data files contain some other programs that runsim/proc_fan fans them out
* Example data in .data files is:
  * <ul>./testsim 5 10</ul>
    <ul>./testsim 8 10</ul>

## How to Run this code:
1. In your command prompt type: make; This will generate the executable files, see below steps
2. Compile with make
* $ make runsim testsim
   * gcc -ggdb -Wall runsim.c -o runsim
   * gcc -ggdb -Wall testsim.c -o testsim

3. Execute
 * <ul> $ ./runsim 2 < fast.data </ul>
  <ul>or</ul>
  <ul> $ ./runsim 2 < testing.data </ul>

## Version Control

 version control:[link](https://github.com/mukka29/Operating-Systems.git)
 or
 https://github.com/mukka29/Operating-Systems.git

