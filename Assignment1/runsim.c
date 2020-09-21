#include <stdio.h>    // used for fgets
#include <stdlib.h>
#include <string.h>   //used for strtok
#include <sys/types.h>
#include <sys/wait.h> //for wait()
#include <unistd.h>   //used for close, fork

static int n = 0; //process count variable
static int N = 0; //processes limit variable

static int check_args(int argc, char * argv[]){

  if(argc != 2){
    fprintf(stderr, "Usage: runsim processes\n");
    return -1;
  }

  if((N = atoi(argv[1])) < 0){
    return -1;
  }
  return 0;
}

static int spawn(char * line){
  int i = 0;
  char *toks[11]; //10 arguments for exec + 1 for NULL


  toks[0]  = strtok(line, " ");
  toks[10] = NULL;

  for(i=1; i < 10; i++){
    toks[i] = strtok(NULL, " ");
  }

  //creating a child process
  if(fork() == 0){
    close(0);
    close(1);

    execvp(toks[0], toks);

    perror("execvp");
    exit(EXIT_FAILURE);
  }

  return 1;
}

static void wait_all(int flags){
  int status;
  while(waitpid(-1, &status, flags) > 0){
    --n;
  }
}

static int read_input(){
  int status;
  char line[100];

  while(1){

    //in case if we have reached the process limit
    if(n >= N){
      wait(&status);
      n--;
    }

    //to get input
    if(fgets(line, sizeof(line), stdin) == NULL){
      break;  //if error or end of input
    }

    //to run the process
    if(spawn(line) > 0){
      n++;  //for one more process
    }

    //to check if any process is exited
    wait_all(WNOHANG);
  }

  return 0;
}

int main(const int argc, char * argv[]){

  if(check_args(argc, argv) == -1){
    return EXIT_FAILURE;
  }

  read_input();
  wait_all(0);

  return EXIT_SUCCESS;
}
