#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

static int T = 0; //sleep time
static int R = 0; //repeats

static int check_args(int argc, char * argv[]){

  //if we don't have 2 arguments
  if(argc != 3){
    fprintf(stderr, "Usage: testsim sleeptime repeats\n");
    return -1;  //error
  }

  //if sleep time is invalid
  if( (T = atoi(argv[1])) < 0){
    return -1;  //error
  }

  //if repeat time is invalid
  if( (R = atoi(argv[2])) < 0){
    return -1;  //error
  }

  return 0;
}

static void testsim(void){
  const pid_t pid = getpid();

  //repeat
  while(R-- > 0){
    sleep(T); //sleep
    fprintf(stderr, "%i\n", pid); //print
  }
}

int main(int argc, char * argv[]){

  if(check_args(argc, argv) == -1){
    return EXIT_FAILURE;
  }

  testsim();

  return EXIT_SUCCESS;
}
