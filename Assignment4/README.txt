1. Compile with make
$ make
gcc -Werror -ggdb -c queue.c
gcc -Werror -ggdb oss.c queue.o -o oss
gcc -Werror -ggdb user.c -o user

2. Execute the program
  $ ./oss

  #Master output is in log.txt
  $ cat log.txt
