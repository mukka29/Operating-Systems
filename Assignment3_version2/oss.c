//header files
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>

#include "data.h"

struct option{
  int val;  //the current value
  int max;  //and the maximum value
};
/*  Most of the code for this project is from project 2 i.e., Assignment 2 as per this repository */

struct option started, running, runtime;  //-n, -s, -t options
struct option exited;  //for the processes which are exited

static int mid=-1, qid = -1;
static struct data *data = NULL;

static int loop_flag = 1;
static struct timeval timestep, temp;

//"signal_running" is used to send a signal to all currently running user processes
static int signal_running(){

  sigset_t mask;
	sigset_t orig_mask;

  sigemptyset (&mask);
	sigaddset (&mask, SIGTERM);

  //block SIGTERM in master
	if(sigprocmask(SIG_BLOCK, &mask, &orig_mask) < 0) {
		perror ("sigprocmask");
		return 1;
	}

  //send SIGTERM to all processes in the process group
  if(killpg(getpid(), SIGTERM) == -1){
    perror("killpg");
    return -1;
  }

  //wait for all processes
  /*int i, status;
  for(i=0; i < running.val; ++i){
    wait(&status);
  }*/

  //unblock SIGTERM in master
  if(sigprocmask(SIG_SETMASK, &orig_mask, NULL) < 0) {
		perror ("sigprocmask");
		return 1;
	}

  return 0;
}

//Exiting master cleanly
static void clean_exit(const int code){

  //stop users
  signal_running();

  printf("[Master: Done (exit %d) at %lu:%i\n",  code, data->timer.tv_sec, data->timer.tv_usec);

  //clean shared memory and th esemaphores
  shmctl(mid, IPC_RMID, NULL);
  msgctl(qid, 0, IPC_RMID);
  shmdt(data);

	exit(code);
}

//Spawning a user program
static int spawn_user(){

	pid_t pid = fork();
  if(pid == -1){
    perror("fork");

  }else if(pid == 0){

    //set the child process group, to be able to receive signals from master
    setpgid(getpid(), getppid());

    execl("user", "user", NULL);
    perror("execl");
    exit(EXIT_FAILURE);

  }else{
    ++running.val;
    printf("Master: Spawned user with PID=%i at system clock %li:%i\n", pid, data->timer.tv_sec, data->timer.tv_usec);
  }

	return pid;
}

//this shows the usage menu in this project
static void usage(){
  printf("Usage: master [-c 5] [-l log.txt] [-t 20]\n");
  printf("\t-h\t Show this message\n");
  printf("\t-c 5\tMaximum processes to be started\n");
  printf("\t-l log.txt\t Log file \n");
  printf("\t-t 20\t Maximum runtime\n");
}

//Set options specified as program arguments
static int set_options(const int argc, char * const argv[]){

  //these are the default options, used as in project 2
  started.val = 5; started.max = 100; //for the maximum users started
  exited.val  = 0; exited.max = 100;
  runtime.val = 0; runtime.max = 20;  //for the maximum runtime in real seconds

  int c, redir=0;
	while((c = getopt(argc, argv, "hc:l:t:")) != -1){
		switch(c){
			case 'h':
        usage();
				return -1;
      case 'c':  started.val	= atoi(optarg); break;
			case 'l':
        stdout = freopen(optarg, "w", stdout);
        redir = 1;
        break;
      case 't':  runtime.max	= atoi(optarg); break;
			default:
				printf("Error: Option '%c' is invalid\n", c);
				return -1;
		}
	}

  if(redir == 0){
    stdout = freopen("log.txt", "w", stdout);
  }
  return 0;
}

//Attaching the data in shared memory
static int attach_data(){

	key_t k1 = ftok(PATH_KEY, MEM_KEY);
	key_t k2 = ftok(PATH_KEY, MSG_KEY);
	if((k1 == -1) || (k2 == -1)){
		perror("ftok");
		return -1;
	}

  //creating the shared memory area
  const int flags = IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR;
	mid = shmget(k1, sizeof(struct data), flags);
  if(mid == -1){
  	perror("shmget");
  	return -1;
  }

  //creating the message queue
	qid = msgget(k2, flags);
  if(qid == -1){
  	perror("msgget");
  	return -1;
  }

  //attaching to the shared memory area
	data = (struct data *) shmat(mid, NULL, 0);
	if(data == NULL){
		perror("shmat");
		return -1;
	}

  //clearing the memory
  bzero(data, sizeof(struct data));

	return 0;
}

//Wait all finished user programs
static void reap_zombies(){
  pid_t pid;
  int status;

  //while we have a process, that exited
  while((pid = waitpid(-1, &status, WNOHANG)) > 0){

    if (WIFEXITED(status)) {
      printf("Master: Child %i exited code %d at system time %li:%i\n", pid, WEXITSTATUS(status), data->timer.tv_sec, data->timer.tv_usec);
    }else if(WIFSIGNALED(status)){
      printf("Master: Child %i signalled with %d at system time %li:%i\n", pid, WTERMSIG(status), data->timer.tv_sec, data->timer.tv_usec);
    }

    --running.val;
    if(++exited.val >= started.max){
      printf("Master: All children exited.\n");
      loop_flag = 0;
    }
  }
}

//Process signals sent to the master
static void sig_handler(const int signal){

  switch(signal){
    case SIGINT:  //for the interrupt signal
      printf("Master: Signal TERM received at %lu:%i\n", data->timer.tv_sec, data->timer.tv_usec);
      loop_flag = 0;  //stop master loop
      break;

    case SIGALRM: //creates an alarm - end of runtime
      printf("Master: Signal ALRM received at %lu:%i\n", data->timer.tv_sec, data->timer.tv_usec);
      loop_flag = 0;
      break;

    case SIGCHLD: //for user program exited
      reap_zombies();
      break;

    default:
      break;
  }
}

//Accepting a single message and sending reply to user who sent it
static int process_msg(){
  struct msgbuf mbuf;
  static int next_msg = MSG_ALL;
  int rv = 0;

  //accepting the message
  if(msgrcv(qid, (void*)&mbuf, MSGBUF_SIZE, next_msg, 0) == -1){
		perror("msgrcv");
		return -1;
	}

  //checking message type
  switch(mbuf.mtype){
    case MSG_ENTER:	//if a user enters the critical section
      next_msg = MSG_LEAVE;	//reading next message only from him (the same sender)
      break;

    case MSG_LEAVE:
      //when a user leaves, critical section is amde open for everybody
      next_msg = MSG_ALL; //reading messages from everybody
      break;

    case MSG_EXIT:  //when a user program terminates

      next_msg = MSG_ALL;	//again reading messages from everybody
      printf("Master: User %d exited at system time %d.%d\n",
        data->user_pid, data->timer.tv_sec, data->timer.tv_usec);
      data->user_pid = 0;

      if(started.val < started.max){
        spawn_user();
        ++started.val;
      }else{  //Now we have generated all of the children
        rv = -1;
      }
      break;

    default:  //for any unknown message
      break;
  }

  //clock keeps on moving forward
  timeradd(&data->timer, &timestep, &temp);
  data->timer = temp;
  if(data->timer.tv_sec >= 2){
    printf("Master: Reach maximum of 2 simulated seconds\n");
    rv = -1;
  }

  //returning message to the sender
	mbuf.mtype = mbuf.pid;  //user will check messages only with his PID
	mbuf.pid   = getpid(); //set the sender of message
	if(msgsnd(qid, &mbuf, MSGBUF_SIZE, 0) == -1){
		perror("msgsnd");
		return -1;
	}

	return rv;
}

int main(const int argc, char * const argv[]){

  if( (attach_data() < 0)||
      (set_options(argc, argv) < 0)){
      clean_exit(EXIT_FAILURE);
  }

  //the clock step
  timestep.tv_sec = 0;
  timestep.tv_usec = 100;

  //ignore these signals to avoid interrupts in msgrcv (message receive)
  signal(SIGINT, SIG_IGN);
  signal(SIGTERM, SIG_IGN);
  signal(SIGCHLD, SIG_IGN);
  signal(SIGALRM, sig_handler);

  alarm(runtime.max);

  //starting the first batch of users
  int i;
  for(i=started.val; i > 0 ; i--){
    spawn_user();
  }

	while(loop_flag){
    if(process_msg() < 0){
      break;
    }
	}

	clean_exit(EXIT_SUCCESS);
}
