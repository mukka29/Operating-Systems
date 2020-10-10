
# Concurrent Unix Processes and Shared Memory
  
## Task:
1. As Kim knew from her English courses, palindromes are phrases that read the same way right-to-left as they do left-to-right, when disregarding capitalization. Being the nerd she is, she likened that to her hobby of looking for palindromes on her vehicle odometer: 245542 (her old car), or 002200 (her new car). Now, in her first job after completing her undergraduate program in Computer Science, Kim is once again working with Palindromes. As part of her work, she is exploring alternative security encoding algorithms based on the premise that the encoding strings are not palindromes. Her first task is to examine a large set of strings to identify those that are palindromes, so that they may be deleted from the list of potential encoding strings. The strings consist of letters, (a, . . . , z, A, . . . Z), and numbers (0, . . . , 9). Ignore any punctuations and spaces. Kim has a one-dimensional array of pointers, mylist[], in which each element points to the start of a string (You can use string class of C++ if you so desire, but I think it will be easier to use C strings for this project). Each string terminates with the NULL character. Kim’s program is to examine each string and print out all strings that are palindromes in one file while the non-palindromes are written to another file.
2. Your job is obviously to write the code for Kim. The main program will read the list of palindromes from a file (one string per line) into shared memory and fork off processes. Each child will test the string at the index assigned to it and write the string into appropriate file, named palin.out and nopalin.out. In addition, the processes will write into a log file (described below). You will have to use the code for multiple process ipc problem (Solution 4 in notes) to protect the critical resources – the two files.
3. Make sure you never have more than 20 processes in the system at any time, even if the program is invoked with n > 20. Add the pid of the child to the file as comment in the log file. The preferred output format for log file is:
* PID Index String

where Index is the logical number for the child, assigned internally by your code, and varies between 0 and n − 1. The child process will be execed by the command
* palin xx
where xx is the index number of the string to be tested in shared memory. You can supply other parameters in exec as needed, such as the logical number of the child. If a process starts to execute code to enter the critical section, it must print a message to that effect on stderr. It will be a good idea to include the time when that happens. Also, indicate the time on stderr when the process actually enters and exits the critical section. Within the critical section, wait for [0-2] seconds before you write into the file.

4. The code for each child process should use the following template:
<ul>
determine if the string is a palindrome;</ul>
 <ul>Concurrent Unix Processes and shared memory 2</ul>
    <ul>execute code to enter critical section;</ul>
<ul>    sleep for random amount of time (between 0 and 2 seconds);</ul>
   <ul> /* Critical section */</ul>
<ul>    execute code to exit from critical section;</ul>

5. You will be required to create separate child processes from your main process, called master. That is, the main process will just spawn the child processes and wait for them to finish. master also sets a timer at the start of computation to 100 seconds. If computation has not finished by this time, master kills all the child processes and then exits. Make sure that you print appropriate message(s)
6. In addition, master should print a message when an interrupt signal (^C) is received. All the children should be killed as a result. All other signals are ignored. Make sure that the processes handle multiple interrupts correctly. As a precaution, add this feature only after your program is well debugged.
7. The code for and child processes should be compiled separately and the executables be called master and palin.
  * Other points to remember: You are required to use fork, exec (or one of its variants), wait, and exit to manage multiple processes. Use shmctl suite of calls for shared memory allocation. Also make sure that you do not have more than twenty processes in the system at any time. You can do this by keeping a counter in master that gets incremented by fork and decremented by wait.

## Termination Criteria: 
1. There are several termination criteria. First, if all the children have finished, the parent process should deallocate shared memory and terminate.
2. In addition, I expect your program to terminate after the specified amount of time (-t option) of real clock. This can be done using a timeout signal, at which point it should kill all currently running child processes and terminate. It should also catch the ctrl-c signal, free up shared memory and then terminate all children. No matter how it terminates, master should also output the value of the shared clock to the output file. For an example of a periodic timer interrupt, you can look at p318 in the text, in the program periodicasterik.c.

## Compilation Steps:
Navigate to the directory "Assignment2" and issue the below commands
<ul>$ gcc -Werror -ggdb master.c -o master</ul>
<ul>$ gcc -Werror -ggdb palin.c -o palin</ul>

## Execution: 
<ul>./master -n 10</ul>

## Checking the result:
* Once the program is ececuted, required Executable files are generated. Along with them, two files called palin.out and nopalin.out will also be generated. The palin.out contains all the words that are palindromes and nopalin.out contains thw words which are not palindromes.
* ls
* this command shows all the files present in the directory after the assignment is executed

## infile.txt File

* This is the text file that I have used for input, it contains many words which are palindromes, non-palindromes and some numbers.

## data.h File
* this contains the constants used and the timeval, with the help of #include <time.h> header file

## log File: output.log
* After the execution, A logfile is generated showing the process logs with timeval
