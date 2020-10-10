//header files
#include <stdio.h>
#include <stdlib.h>
#include <signal.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <sys/wait.h>
#include <sys/time.h>
#include "data.h"

struct option{
  int val;  //current value
  int max;  //maximum value
};

struct option started, running, runtime;  //-n, -s, -t options
struct option exited, wi; //exited processes and the word index

static int mid=-1, sid = -1;
static struct data *data = NULL;

static int loop_flag = 1;

//Sending a signal to all running palin processes
static int signal_running(){

  sigset_t mask;
	sigset_t orig_mask;

  if(running.val <= 0){
    return 0;
  }

  sigemptyset (&mask);
	sigaddset (&mask, SIGTERM);

  //blocking SIGTERM in master
	if(sigprocmask(SIG_BLOCK, &mask, &orig_mask) < 0) {
		perror ("sigprocmask");
		return 1;
	}

  //sending SIGTERM to all processes in our process group
  if(killpg(getpid(), SIGTERM) == -1){
    perror("killpg");
    return -1;
  }

  //unblocking SIGTERM in master
  if(sigprocmask(SIG_SETMASK, &orig_mask, NULL) < 0) {
		perror ("sigprocmask");
		return 1;
	}

  //waiting for all processes
  int i, status;
  for(i=0; i < running.val; ++i){
    wait(&status);
  }
  return 0;
}

//Exit master cleanly
void clean_exit(const int code){

  //stop the children
  signal_running();

  printf("[MASTER|%li:%i] Done (exit %d)\n", data->timer.tv_sec, data->timer.tv_usec, code);

  //clean the shared memory and semaphores
  shmctl(mid, IPC_RMID, NULL);
  semctl(sid, 0, IPC_RMID);
  shmdt(data);

	exit(code);
}

//Spawn a palin program
static int spawn_palin(){

	pid_t pid = fork();
  if(pid == -1){
    perror("fork");

  }else if(pid == 0){

    char index[10];
    snprintf(index, 10, "%u", wi.val);

    //set child process group, so to be able to receive signals form master
    setpgid(getpid(), getppid());

    execl("palin", "palin", index, NULL);
    perror("execl");
    exit(EXIT_FAILURE);

  }else{
    ++started.val;
    ++running.val;
    printf("[MASTER|%li:%i] Spawned palin with PID=%i and wi=%d\n", data->timer.tv_sec, data->timer.tv_usec, pid, wi.val);
    ++wi.val;
  }

	return pid;
}

//Show usage menu
static void usage(){
  printf("Usage: master [-n x] [-s x] [-t x] infile.txt\n");
  printf("\t-h\t Show this message\n");
  printf("\t-n x\tMaximum processes to be started\n");
  printf("\t-s x\t Maximum processes running at once, [1;20]\n");
  printf("\t-t x\t Maximum processes running at once, [1;20]\n");
}

//Insert words from input file in to shared memory array
static int insert_words(const char * filename){

  //opening the input file
  FILE * infile = fopen(filename, "r");
  if(infile == NULL){
    perror("fopen");
    return -1;
  }

  //reading each line to mylist[] array
  int i = 0;
  while(fgets(data->mylist[i++], WORD_MAX, infile) != NULL){
    if(i > LIST_MAX){ //if we have filled array
      fprintf(stderr, "Error: Only %d words are supported\n", LIST_MAX);
      break;
    }
  }
  fclose(infile);
  return i;
}

//Set options specified as program arguments
static int set_options(const int argc, char * const argv[]){

  //default options
  started.val = 0; started.max = 4;
  running.val = 0; running.max = 2;
  exited.val  = 0; exited.max = 0;
  runtime.val = 0; runtime.max = 100;
  wi.val = wi.max = 0;

  int c;
	while((c = getopt(argc, argv, "hn:s:t:")) != -1){
		switch(c){
			case 'h':
        usage();
				return -1;
      case 'n':  started.max	= atoi(optarg); break;
			case 's':  running.max	= atoi(optarg); break;
      case 't':  runtime.max	= atoi(optarg); break;
			default:
				fprintf(stderr, "Error: Option '%c' is invalid\n", c);
				return -1;
		}
	}

  //checking the running value
  if( (running.max <= 0) || (running.max > 20)   ){
    fprintf(stderr, "Error: -s invalid\n");
    return -1;
  }
  exited.max = started.max;

  //set input file
  if(optind == (argc -1)){
    wi.max = insert_words(argv[argc-1]);
  }else{
    wi.max = insert_words("infile.txt");
  }
  if(wi.max <= 0){
    clean_exit(EXIT_FAILURE);
  }

  return 0;
}

//Attach the data in shared memory
static int attach_data(){

	key_t k1 = ftok(PATH_KEY, MEM_KEY);
	key_t k2 = ftok(PATH_KEY, SEM_KEY);
	if((k1 == -1) || (k2 == -1)){
		perror("ftok");
		return -1;
	}

  const int flags = IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR;
	mid = shmget(k1, sizeof(struct data), flags);
  if(mid == -1){
  	perror("semget");
  	return -1;
  }

	sid = semget(k2, 2, flags);
  if(sid == -1){
  	perror("semget");
  	return -1;
  }

	data = (struct data *) shmat(mid, NULL, 0);
	if(data == NULL){
		perror("shmat");
		return -1;
	}
  bzero(data, sizeof(struct data));

  //set the semaphores
  unsigned short vals[2] = {1,1};
  union semun un;
  un.array = vals;

	if(semctl(sid, 0, SETALL, un) ==-1){
		perror("semctl");
		return -1;
	}

	return 0;
}

//Wait all finished palin programs
static void reap_zombies(){
  pid_t pid;
  int status;

  //while we have a process, that is exited
  while((pid = waitpid(-1, &status, WNOHANG)) > 0){

    if (WIFEXITED(status)) {
      printf("[MASTER|%li:%i] Child %i exited code %d\n", data->timer.tv_sec, data->timer.tv_usec, pid, WEXITSTATUS(status));
    }else if(WIFSIGNALED(status)){
      printf("[MASTER|%li:%i] Child %i signalled with %d\n", data->timer.tv_sec, data->timer.tv_usec, pid, WTERMSIG(status));
    }

    --running.val;
    if(++exited.val >= started.max){
      loop_flag = 0;
    }
  }
}

//Process signals sent to master
static void sig_handler(const int signal){

  switch(signal){
    case SIGINT:  //interrupt signal
      printf("[MASTER|%li:%i] Signal TERM received\n", data->timer.tv_sec, data->timer.tv_usec);
      loop_flag = 0;  //stop master loop
      break;

    case SIGALRM: //alarm - end of runtime
      printf("[MASTER|%li:%i] Signal ALRM received\n", data->timer.tv_sec, data->timer.tv_usec);
      loop_flag = 0;
      break;

    case SIGCHLD: //palin program exited
      reap_zombies();
      break;

    default:
      break;
  }
}

int main(const int argc, char * const argv[]){

  stdout = freopen("output.log", "w", stdout);

  struct timeval timestep, temp;
  if( (attach_data() < 0)||
      (set_options(argc, argv) < 0)){
      clean_exit(EXIT_FAILURE);
  }

  //this is the clock step
  timestep.tv_sec = 0;
  timestep.tv_usec = 1000;

  if(started.max > wi.max){ //limit upto the max words in file
    started.max = wi.max;
  }

  signal(SIGINT, SIG_IGN);
  signal(SIGTERM, sig_handler);
  signal(SIGCHLD, sig_handler);
  signal(SIGALRM, sig_handler);

  alarm(runtime.max);

	while(loop_flag && (started.val < started.max)){

    //if we can, we now start a new process
    if( (wi.val < wi.max) &&
        (started.val < started.max) &&
        (running.val < running.max)  ){

        pid_t pid = spawn_palin();
    }

    //clock keeps moving forward
    timeradd(&data->timer, &timestep, &temp);
    data->timer = temp;
	}

	clean_exit(EXIT_SUCCESS);
}
