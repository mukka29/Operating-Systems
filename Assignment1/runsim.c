#include <stdio.h>    // for fgets
#include <stdlib.h>
#include <string.h>   //for strtok
#include <sys/types.h>
#include <sys/wait.h> //for wait()
#include <unistd.h>   //for close, fork

static int n = 0; //process count
static int N = 0; //processes limit

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

  //create a child process
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

    //if we have reached the process limit
    if(n >= N){
      wait(&status);
      n--;
    }

    //get input
    if(fgets(line, sizeof(line), stdin) == NULL){
      break;  //error or end of input
    }

    //run the process
    if(spawn(line) > 0){
      n++;  //one more process
    }

    //check if any process exited
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
