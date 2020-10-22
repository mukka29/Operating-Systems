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

struct option started, running, runtime;  //-n, -s, -t options
struct option exited; //processes which are exited

static int mid=-1, qid = -1;
static struct data *data = NULL;

static int loop_flag = 1;
static struct timeval timestep, temp;

//"signal_running" is used to send a signal to all running user processes
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

  //send SIGTERM to all processes in our process group
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

//Exit master cleanly
static void clean_exit(const int code){

  //stop users
  signal_running();

  printf("[MASTER|%li:%i] Done (exit %d)\n", data->timer.tv_sec, data->timer.tv_usec, code);

  //clean shared memory and semaphores
  shmctl(mid, IPC_RMID, NULL);
  msgctl(qid, 0, IPC_RMID);
  shmdt(data);

	exit(code);
}

//Spawn a user program
static int spawn_user(){

	pid_t pid = fork();
  if(pid == -1){
    perror("fork");

  }else if(pid == 0){

    //set child process group, to be able to receive master signals
    setpgid(getpid(), getppid());

    execl("user", "user", NULL);
    perror("execl");
    exit(EXIT_FAILURE);

  }else{
    ++running.val;
    printf("[MASTER|%li:%i] Spawned user with PID=%i\n", data->timer.tv_sec, data->timer.tv_usec, pid);
  }

	return pid;
}

//Show usage menu
static void usage(){
  printf("Usage: master [-c 5] [-l log.txt] [-t 20]\n");
  printf("\t-h\t Show this message\n");
  printf("\t-c 5\tMaximum processes to be started\n");
  printf("\t-l log.txt\t Log file \n");
  printf("\t-t 20\t Maximum runtime\n");
}

//Set options specified as program arguments
static int set_options(const int argc, char * const argv[]){

  //default options
  started.val = 5; started.max = 100; //max users started
  exited.val  = 0; exited.max = 100;
  runtime.val = 0; runtime.max = 20;  //maximum runtime in real seconds

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

//Attach the data in shared memory
static int attach_data(){

	key_t k1 = ftok(PATH_KEY, MEM_KEY);
	key_t k2 = ftok(PATH_KEY, MSG_KEY);
	if((k1 == -1) || (k2 == -1)){
		perror("ftok");
		return -1;
	}

  //create the shared memory area
  const int flags = IPC_CREAT | IPC_EXCL | S_IRUSR | S_IWUSR;
	mid = shmget(k1, sizeof(struct data), flags);
  if(mid == -1){
  	perror("shmget");
  	return -1;
  }

  //create the message queue
	qid = msgget(k2, flags);
  if(qid == -1){
  	perror("msgget");
  	return -1;
  }

  //attach to the shared memory area
	data = (struct data *) shmat(mid, NULL, 0);
	if(data == NULL){
		perror("shmat");
		return -1;
	}

  //clear the memory
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
      printf("[MASTER|%li:%i] Child %i exited code %d\n", data->timer.tv_sec, data->timer.tv_usec, pid, WEXITSTATUS(status));
    }else if(WIFSIGNALED(status)){
      printf("[MASTER|%li:%i] Child %i signalled with %d\n", data->timer.tv_sec, data->timer.tv_usec, pid, WTERMSIG(status));
    }

    --running.val;
    if(++exited.val >= started.max){
      printf("Master: All children exited.\n");
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

    case SIGCHLD: //user program exited
      reap_zombies();
      break;

    default:
      break;
  }
}

//Accept a single message and send reply to user who sent it
static int process_msg(){
  struct msgbuf mbuf;
  static int next_msg = MSG_ALL;
  int rv = 0;

  //accept the message
  if(msgrcv(qid, (void*)&mbuf, MSGBUF_SIZE, next_msg, 0) == -1){
		perror("msgrcv");
		return -1;
	}

  //check message type
  switch(mbuf.mtype){
    case MSG_ENTER:	//if a user enters the critical section
      next_msg = MSG_LEAVE;		//read next message only from him
      break;

    case MSG_LEAVE:
      //when a user leaves, critical section is open for everybody
      next_msg = MSG_ALL;		   //read messages from everybody
      break;

    case MSG_EXIT:  //a user program terminates

      next_msg = MSG_ALL;		   //read messages from everybody
      printf("[MASTER] User %d exited at system time %ld.%d\n",
        data->user_pid, data->timer.tv_sec, data->timer.tv_usec);
      data->user_pid = 0;

      if(started.val < started.max){
        spawn_user();
        ++started.val;
      }else{  //we have generated all of the children
        rv = -1;
      }
      break;

    default:  //unknown message
      break;
  }

  //clock moves forward
  timeradd(&data->timer, &timestep, &temp);
  data->timer = temp;
  if(data->timer.tv_sec >= 2){
    printf("Master: Reach maximum of 2 simulated seconds\n");
    rv = -1;
  }

  //return message to sender
	mbuf.mtype = mbuf.pid;  //user checks only messages with his PID
	mbuf.pid   = getpid(); //set sender of message
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

  //our clock step
  timestep.tv_sec = 0;
  timestep.tv_usec = 100;

  //ignore signals to avoid interrupts in msgrcv
  signal(SIGINT, SIG_IGN);
  signal(SIGTERM, SIG_IGN);
  signal(SIGCHLD, SIG_IGN);
  signal(SIGALRM, sig_handler);

  alarm(runtime.max);

  //start the first batch of users
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
