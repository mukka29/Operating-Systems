//header files
#include <unistd.h>
#include <string.h>
#include <stdlib.h>
#include <signal.h>
#include <stdio.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <fcntl.h>
#include <sys/ipc.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <sys/time.h>
#include <errno.h>

#include "queue.h"

/*  Most of the code for this project is from project 3 and 4 i.e., Assignment 3 and 4 as per this repository */

struct option{
  int val;  //the current value
  int max;  //and the maximum value
};

struct rstat{
  int num_req;  //total requests
  int num_ret;  //total returns
  int num_ret_all;  //total return all
  int num_deny; //total requests denied / total deadlocks
  int num_lines;
} rstat;

struct option started, running, runtime;  //-n, -s, -t options
struct option exited;	//for the processes which are exited and for word index
static int verbose = 0;

static int mid=-1, qid = -1, sid = -1;
static struct data *data = NULL;

static int loop_flag = 1;
static struct timeval timestep;

static queue_t blocked; //for the blocked queue

//represents the bit vector for user[]
static unsigned int bitvector = 0; 

static int critical_lock(){
	struct sembuf sop;

  sigset_t mask, oldmask;
  sigemptyset(&mask);
  sigaddset(&mask, SIGCHLD);
  sigprocmask(SIG_BLOCK, &mask, &oldmask);

	sop.sem_num = 0;
	sop.sem_flg = 0;
	sop.sem_op  = -1;
	if(semop(sid, &sop, 1) == -1) {
		 perror("semop");
     sigprocmask(SIG_SETMASK, &oldmask, NULL);
		 return -1;
	}
  sigprocmask(SIG_SETMASK, &oldmask, NULL);
	return 0;
}

//Unlocking the critical section
static int critical_unlock(){
	struct sembuf sop;
	sop.sem_num = 0;
	sop.sem_flg = 0;
	sop.sem_op  = 1;
	if(semop(sid, &sop, 1) == -1) {
		 perror("semop");
		 return -1;
	}
	return 0;
}

// Returning bit value
static int check_bit(const int b){
  return ((bitvector & (1 << b)) >> b);
}

// Switching bit value
static void switch_bit(const int b){
  bitvector ^= (1 << b);
}

// Finding the unset bit
static int find_bit(){
  int i;
  for(i=0; i < RUNNING_MAX; i++){
    if(check_bit(i) == 0){
      return i;
    }
  }
  return -1;
}

// Incrementing the timer
static void timerinc(struct timeval *a, struct timeval * inc){
  struct timeval res;
  critical_lock();
  timeradd(a, inc, &res);
  *a = res;
  critical_unlock();
}

//Wait all finished user programs
static void reap_zombies(){
  pid_t pid;
  int status, i;

  //while we have a process, that exited
  while((pid = waitpid(-1, &status, WNOHANG | WUNTRACED | WCONTINUED)) > 0){

    if (WIFEXITED(status)) {
      printf("Master: Child %i exited code %d at systm time %lu:%li\n", pid, WEXITSTATUS(status), data->timer.tv_sec, data->timer.tv_usec);
    }else if(WIFSIGNALED(status)){
      printf("Master: Child %i signalled with %d at systm time %lu:%li\n", pid, WTERMSIG(status), data->timer.tv_sec, data->timer.tv_usec);
    }
    rstat.num_lines++;

    --running.val;

    for(i=0; i < RUNNING_MAX; i++){
      if(pid == data->users[i].pid){
        switch_bit(i);
        bzero(&data->users[i], sizeof(struct user_pcb));
        break;
      }
    }

    //fprintf(stderr, "Reaped %d\n", pid);
    if(++exited.val >= started.max){
      printf("OSS: All children exited\n");
      loop_flag = 0;
    }
  }
}

//Exiting master cleanly
static void clean_exit(const int code){

  if(data){
    int i;
    struct msgbuf mbuf;
    sigset_t mask, oldmask;

    sigemptyset(&mask);
  	sigaddset(&mask, SIGCHLD);
    sigprocmask(SIG_BLOCK, &mask, &oldmask);

//sending empty slice to all running processes for termination
    while(running.val > 0){
//some user may be waiting on timer
      data->timer.tv_sec++;

      for(i=0; i < RUNNING_MAX; i++){

        critical_lock();
        if((data->users[i].request.state == WAITING) ||
           (data->users[i].request.state == BLOCKED) ){

          data->users[i].request.state = DENIED;

          mbuf.mtype = data->users[i].pid;
          mbuf.user_id = -1; //user will quit on id=-1
          if(msgsnd(qid, &mbuf, MSGBUF_SIZE, 0) == -1){
            perror("msgsnd");
          }
        }
        critical_unlock();
      }
      reap_zombies();
      //  fprintf(stderr, "%d left\n", running.val);
    }
    sigprocmask(SIG_SETMASK, &oldmask, NULL);
  }

  printf("Master:  Done (exit %d)at system time %lu:%li\n", code, data->timer.tv_sec, data->timer.tv_usec);

  q_deinit(&blocked);

  //cleaning shared memory and the semaphores
  shmctl(mid, IPC_RMID, NULL);
  msgctl(qid, 0, IPC_RMID);
  semctl(sid, 0, IPC_RMID);
  shmdt(data);

	exit(code);
}

//Spawn a user program
static int spawn_user(){
  char user_id[10];

  const int id = find_bit();
  if(id == -1){
    //fprintf(stderr, "No bits. Running=%d\n", running.val);
    return -1;
  }
  struct user_pcb * usr = &data->users[id];

	const pid_t pid = fork();
  if(pid == -1){
    perror("fork");

  }else if(pid == 0){

    snprintf(user_id, sizeof(user_id), "%d", id);

    execl("user", "user", user_id, NULL);
    perror("execl");
    exit(EXIT_FAILURE);

  }else{
    ++running.val;
    switch_bit(id);
    //fprintf(stderr, "Running=%d\n", running.val);

    usr->pid = pid;
    usr->id	= started.val++;
    usr->state = ST_READY;
    printf("OSS: Generating process with PID %u at system time %lu:%li\n", usr->id, data->timer.tv_sec, data->timer.tv_usec);
  }

	return pid;
}

//this shows the usage menu in this project
static void usage(){
  printf("Usage: master [-v] [-c 5] [-l log.txt] [-t 20]\n");
  printf("\t-h\t Show this message\n");
  printf("\t-c 5\tMaximum processes to be started\n");
  printf("\t-l log.txt\t Log file \n");
  printf("\t-t 20\t Maximum runtime\n");
}

//Set options specified as program arguments
static int set_options(const int argc, char * const argv[]){

  //these are the default options used as in the previous Projects
  started.val = 0; started.max = 40; //max users started
  exited.val  = 0; exited.max = 40;
  runtime.val = 0; runtime.max = 5;  //maximum runtime in real seconds
  running.val = 0; running.max = RUNNING_MAX;


  int c, redir=0;
	while((c = getopt(argc, argv, "hc:l:t:v")) != -1){
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
      case 'v': verbose = 1; break;
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
  key_t k3 = ftok(PATH_KEY, SEM_KEY);
	if((k1 == -1) || (k2 == -1) || (k3 == -1)){
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

  sid = semget(k3, 1, flags);
  if(sid == -1){
  	perror("semget");
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

  //setting semaphores
  unsigned short val = 1;
	if(semctl(sid, 0, SETVAL, val) ==-1){
		perror("semctl");
		return -1;
	}

	return 0;
}

//Process signals sent to master
static void sig_handler(const int signal){

  switch(signal){
    case SIGINT:  //for interrupt signal
      printf("Master: Signal TERM receivedat system time %lu:%li\n", data->timer.tv_sec, data->timer.tv_usec);
      loop_flag = 0;  //stop master loop
      break;

    case SIGALRM: //creates an alarm - for end of runtime
      printf("Master:  Signal ALRM received at system time %lu:%li\n", data->timer.tv_sec, data->timer.tv_usec);
      loop_flag = 0;
      break;

    case SIGCHLD: //for user program exited
      reap_zombies();
      break;

    default:
      break;
  }
}

static int schedule_blocked(){
  struct msgbuf mbuf;

  int i;
  for(i=0; i < blocked.len; i++){
    const int id = blocked.items[i];
    struct user_pcb * usr = &data->users[id];

    if(data->R[usr->request.id].available >= usr->request.val){ //if we have enough resource
      break;
    }
  }

  int state = ACCEPTED;

  if(i == blocked.len){

    //if all users are in the blocked queue
    if((blocked.len > 0) && (blocked.len == running.val)){
      //they are stuck, and we have to remove one to unblock them
      i = 0;  //and remove the first one
      state = DENIED;
    }else{
      return 0; //nobody to unblock
    }
  }

  const int id = q_deq(&blocked, i);
  struct user_pcb * usr = &data->users[id];

  //when process becomes ready
  usr->state = ST_READY;
  usr->request.state = state;  //change request state to process

  if(state == DENIED){
    rstat.num_deny++;
    printf("OSS: Denied process with PID %d waiting on R%d=%d at system time %lu:%li\n",
      usr->id, usr->request.id, usr->request.val, data->timer.tv_sec, data->timer.tv_usec);
  }else{

    rstat.num_req++;

    printf("OSS: Unblocked process with PID %d waiting on R%d=%d at system time %lu:%li\n",
      usr->id, usr->request.id, usr->request.val, data->timer.tv_sec, data->timer.tv_usec);
  }

  //unblocking the user
  mbuf.mtype = usr->pid;  //set its pid as type
  mbuf.user_id = usr->id;
  if(msgsnd(qid, (void*)&mbuf, MSGBUF_SIZE, 0) == -1){
    perror("msgrcv");
    return -1;
  }

  return 1;
}

static int all_finished(const int finished[RUNNING_MAX]){

  int i, first_unfinished = 0;
	for(i=0; i < RUNNING_MAX; i++){
		if(!finished[i]){
      first_unfinished = 1; //save index in users[] of first unfinished process
			break;
    }
  }

  if(verbose && (first_unfinished > 0)){
    printf("\tProcesses ");
    for(i=0; i < RUNNING_MAX; i++){
  		if(!finished[i]){
        printf("P%d ", data->users[i].id);
      }
    }
    printf("could deadlock in this scenario.\n");
  }

	return first_unfinished;
}

static int deadlock_check(void){

	int i,j, avail[RCOUNT], finished[RUNNING_MAX];

	// nobody is finished
	for(i=0; i < RUNNING_MAX; i++){
		finished[i] = 0;
  }

	// all the available resources
	for(j=0; j < RCOUNT; j++)
		avail[j] = data->R[j].available;

  //locking semaphore to block new requests
  critical_lock();

  i=0;
	while(i != RUNNING_MAX){

		for(i=0; i < RUNNING_MAX; i++){

			if(finished[i] == 1)
				continue;

      struct user_pcb * usr = &data->users[i];

      if(usr->pid == 0){
        finished[i] = 1;
        continue;
      }

      //checking if the claim left, whether it can be satisfied or not
      int claim_ok = 1;
      for(j=0; j < RCOUNT; j++){
        //if available is less than the claim
    		if(data->R[j].available < (usr->r[j].total - usr->r[j].available)){
          claim_ok = 0; //then the claim is not satisfied
        }
      }

      if(	(usr->request.val < 0 ) ||
          (usr->request.state != WAITING) ||
					claim_ok){

				finished[i] = 1;

        //return user allocated to available
				for(j=0; j < RCOUNT; j++){
					avail[j] += usr->r[j].available;
				}
				break;
			}
		}
	}
  critical_unlock();

  // returning index of first user in deadlock, if any
	return all_finished(finished);
}

static int schedule_dispatch(){
  struct msgbuf mbuf;
  struct timeval tv;
  sigset_t mask, oldmask;

  sigemptyset(&mask);
	sigaddset(&mask, SIGCHLD);
  sigprocmask(SIG_BLOCK, &mask, &oldmask);

  if(msgrcv(qid, (void*)&mbuf, MSGBUF_SIZE, getpid(), IPC_NOWAIT) == -1){
    if(errno != ENOMSG){
		    perror("msgrcv");
    }
    sigprocmask(SIG_SETMASK, &oldmask, NULL);
		return -1;
	}
  sigprocmask(SIG_SETMASK, &oldmask, NULL);

  struct user_pcb * usr = &data->users[mbuf.user_id];
  printf("OSS: Acknowledged process with PID %u is making a request for R%d at system time %lu:%li\n",
    usr->id, usr->request.id, data->timer.tv_sec, data->timer.tv_usec);

  //processing the request
  int reply = 0;
  if(usr->request.val < 0){//incase if user wants to release

    usr->request.state = ACCEPTED;  //the request is accepted
    reply = 1;
    //and resource is returned to the system
    data->R[usr->request.id].available += -1*usr->request.val;

    printf("OSS: Acknowledged process with PID %u freed R%d=%d at system time %lu:%li\n",
      usr->id, usr->request.id, -1*usr->request.val, data->timer.tv_sec, data->timer.tv_usec);

    rstat.num_ret++;

  }else if(usr->request.id == -1){ //if user is releasing all

    usr->request.state = ACCEPTED;  //request is accepted
    reply = 1;

    //returning all user resources to the system
    printf("OSS: Acknowledged process with PID %u freeing resouces: ", usr->id);
    int i;
    for(i=0; i < RCOUNT; i++){
      if(usr->r[i].available > 0){
        printf("R%d=%d ", i, usr->r[i].available);
      }
    }
    printf("\n");
    rstat.num_ret_all++;

  //when user wants to request a resource
  }else if(data->R[usr->request.id].available >= usr->request.val){ //if we have enough resource

    printf("OSS: Running deadlock avoidance at system time %lu:%li\n", data->timer.tv_sec, data->timer.tv_usec);
    rstat.num_lines++;

    //check if we are in deadlock, after the request
    if(deadlock_check()){

      printf("Unsafe state after granting user; request not granted\n");
      rstat.num_lines++;
  //denying the request to avoid the deadlock
      usr->request.state = DENIED;
      reply = 1;

      rstat.num_deny++;

    }else{  //if the request doesn't lead to deadlock
      printf("\tSafe state after accepting request found\n");
      printf("\tOSS granting R%d=%d to P%d at time %lu:%li\n", usr->request.id, usr->request.val, usr->id,
        data->timer.tv_sec, data->timer.tv_usec);
      rstat.num_lines += 2;

      //removing the requested amount from system resources
      data->R[usr->request.id].available -= usr->request.val;
      usr->r[usr->request.id].available  += usr->request.val;

      usr->request.state = ACCEPTED;
      reply = 1;  //unblock the user
    }

  }else{  //if we don't have the resource

    reply = 0;  //don't unblock user
    usr->request.state = BLOCKED;
    if(q_enq(&blocked, mbuf.user_id) == 0){  //user will wait in the blocked queue
      printf("\tP%d added to wait queue, waiting on R%d=%d\n", usr->id, usr->request.id, usr->request.val);
    }else{
      printf("\tP%d wans't added to wait queue, queue is full\n", usr->id);
    }
    rstat.num_lines++;
  }

  //if we should unblock the user
  if(reply){

    rstat.num_req++;

    mbuf.mtype = usr->pid;  //set its pid as type
    mbuf.user_id = usr->id;
    if(msgsnd(qid, (void*)&mbuf, MSGBUF_SIZE, 0) == -1){
  		perror("msgrcv");
  		return -1;
  	}
  }

  //calculating the dispatch time
  tv.tv_sec = 0; tv.tv_usec = rand() % 100;
  timerinc(&data->timer, &tv);
  printf("OSS: total time this dispatching was %li nanoseconds at system time %lu:%li\n", tv.tv_usec, data->timer.tv_sec, data->timer.tv_usec);
  rstat.num_lines++;

  return 0;
}

static void print_requests(){

  int i;
  printf("OSS: Resource requests at time %li.%li\n", data->timer.tv_sec, data->timer.tv_usec);

  for(i=0; i < RUNNING_MAX; i++){
    struct user_pcb * usr = &data->users[i];
    if((usr->pid > 0) && (usr->request.val != 0)){
      printf("P%d: R%d=%d\n", usr->id, usr->request.id, usr->request.val);
			rstat.num_lines++;
    }
  }
}

static void print_resmap(){

  printf("Currently available resources at time %li.%li\n", data->timer.tv_sec, data->timer.tv_usec);
	rstat.num_lines++;

	printf("    ");
	int i;
	for(i=0; i < RCOUNT; i++){
		printf("R%2d ", i);
  }
	printf("\n");
	rstat.num_lines++;


	//OSS total resources
	printf("TOT ");
  for(i=0; i < RCOUNT; i++){
    printf("%*d ", 3, data->R[i].total);
  }
	printf("\n");
	rstat.num_lines++;

	//OSS avaialble
  printf("OSS ");
  for(i=0; i < RCOUNT; i++){
    printf("%3d ", data->R[i].available);
  }
  printf("\n");
	rstat.num_lines++;


	//showing what the users have
	for(i=0; i < RUNNING_MAX; i++){
    struct user_pcb * usr = &data->users[i];
		if(usr->pid > 0){
  		printf("P%2d ", usr->id);

			int j;
  		for(j=0; j < RCOUNT; j++){
  			printf("%3d ", usr->r[j].available);
			}

  		printf("\n");
			rstat.num_lines++;
    }
	}
}

int main(const int argc, char * const argv[]){

  const int maxTimeBetweenNewProcsSecs = 1;
  const int maxTimeBetweenNewProcsNS = 5000;
  struct timeval timer_fork;

  if( (attach_data() < 0)||
      (set_options(argc, argv) < 0)){
      clean_exit(EXIT_FAILURE);
  }

  //this is the clock step of 100 ns
  timestep.tv_sec = 0;
  timestep.tv_usec = 100;

  //ignoring the signals to avoid interrupts in msgrcv
  signal(SIGINT,  sig_handler);
  signal(SIGTERM, sig_handler);
  signal(SIGCHLD, sig_handler);
  signal(SIGALRM, sig_handler);

  //alarm(runtime.max);

  bzero(&rstat, sizeof(struct rstat));

  q_init(&blocked, started.max);  //initializing the queue
  //allocate and setup resource descriptors
  res_alloc(data->R);

	while(loop_flag){

   //clock keeps on moving forward
    timerinc(&data->timer, &timestep);
    if(timercmp(&data->timer, &timer_fork, >=) != 0){

      if(started.val < started.max){
        spawn_user();
      }else{  //if we have generated all of the children at this stage
        break;
      }

      //setting the next fork time
      timer_fork.tv_sec  = data->timer.tv_sec + (rand() % maxTimeBetweenNewProcsSecs);
      timer_fork.tv_usec = data->timer.tv_usec + (rand() % maxTimeBetweenNewProcsNS);
    }

    schedule_dispatch();
    schedule_blocked();

    if((verbose == 1) && ((rstat.num_req % 20) == 1)){
      print_resmap();
      print_requests();
      fflush(stdout);
    }
	}
/*
printing each of the values of total accepts, total returns, total return all, total denied and grants
total_return is incremented , when user returns one resource
total_return_all is when user releases all resources, at the end.
*/
  printf("Time taken: %lu:%li\n", data->timer.tv_sec, data->timer.tv_usec);

  printf("Total accepts: %d\n", rstat.num_req);
  printf("Total returns: %d\n", rstat.num_ret);
  printf("Total return all: %d\n", rstat.num_ret_all);
  printf("Total denied(deadlocks): %d\n", rstat.num_deny);
  printf("Grants / process: %.1f\n", (float) rstat.num_req / started.val);

	clean_exit(EXIT_SUCCESS);
  return 0;
}
