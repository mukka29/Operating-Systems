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

/*  Most of the code for this project is from the previous projects per this repository */

struct option{
  int val;  //the current value
  int max;  //and the maximum value
};

struct mstat{
  int num_req;  //total requests
  int num_block;
  int num_deny; //total requests denied / total deadlocks
  int num_rd;  //total reads
  int num_wr;  //total writes
  int num_fault;
  int num_evict;
  int num_lines;
} mstat;

struct option started, running, runtime, addr_scheme;  //-n, -s, -t, -m options
struct option exited; //for the processes which are exited and for word index
static int verbose = 0;

static int mid=-1, qid = -1, sid = -1;
static struct data *data = NULL;

static int loop_flag = 1;
static struct timeval timestep;

static queue_t blocked; //for the blocked queue

//represents the bit vector for user[]
static unsigned int bitvector = 0; 
static unsigned char frame_bitvector[(FTBL_SIZE / 8) + 1];

static void switch_bit(const int b){
  bitvector ^= (1 << b);
}

//Finding a frame to be evicted, based on Least Recently used (LRU)
static int eviction_policy(struct frame * frame_tbl){
  int i, evicted = 0;
  for(i=0; i < FTBL_SIZE; i++){
    if(frame_tbl[i].lru > frame_tbl[evicted].lru){
      evicted = i;
    }
  }
  return evicted;

}

static void eviction_update_timestamp(struct frame * frame_tbl, const int f){
  int i;
  for(i=0; i < FTBL_SIZE; i++){
    frame_tbl[i].lru++;
  }
  frame_tbl[f].lru--;
}

static void frame_mark_used(struct frame *frame, const int f){

  frame_bitvector[f / 8] |= (1 << (f % 8));  //set the bit
  frame->state = FR_USED;
}

//Finding the unused frame
static int find_frame(){
	int i;
  for(i=0; i < FTBL_SIZE; i++){
    const int bit = i % 8;
  	if( ((frame_bitvector[i / 8] & (1 << bit)) >> bit) == 0){
      return i;
    }
  }
  return -1;
}

static void frame_reset(struct frame *frame, const int f){
  frame->state = FR_UNUSED;
  frame->p = frame->u = -1;

  frame_bitvector[f / 8] &= ~(1 << (f % 8));
}

static void ptbl_reset(struct virt_page *ptbl, struct frame * ftbl){
  int i;
	for(i=0; i < PTBL_SIZE; i++){

		struct virt_page *vp = &ptbl[i];

    if(vp->f >= 0){
			frame_reset(&ftbl[vp->f], vp->f);
			vp->f = -1;
		}
		vp->ref = 0;
	}
}

static void user_reset(const int user_id){
  int i;
  for(i=0; i < q_len(&blocked); i++){
    if(blocked.items[i] == user_id){
      q_deq(&blocked, i);
    }
  }

  switch_bit(user_id);
  data->users[user_id].pid = 0;
  data->users[user_id].id = 0;
  data->users[user_id].state = ST_READY;

  ptbl_reset(data->users[user_id].PT, data->FT);
}

static void mem_alloc(){
  int i;

  bzero(frame_bitvector, sizeof(frame_bitvector));

  //initializing the page and the frame tables
	for(i=0; i < RUNNING_MAX; i++){
    ptbl_reset(data->users[i].PT, data->FT);
  }

	//clearing the frame table
	for(i=0; i < FTBL_SIZE; i++){
		frame_reset(&data->FT[i], i);
  }
}

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
      printf("Master: Child %i exited code %d at system time %lu:%li\n", pid, WEXITSTATUS(status), data->timer.tv_sec, data->timer.tv_usec);
    }else if(WIFSIGNALED(status)){
      printf("Master: Child %i signalled with %d at system time %lu:%li\n", pid, WTERMSIG(status), data->timer.tv_sec, data->timer.tv_usec);
    }
    mstat.num_lines++;

    --running.val;

    for(i=0; i < RUNNING_MAX; i++){
      if(pid == data->users[i].pid){
        user_reset(i);
        break;
      }
    }

    //fprintf(stderr, "Reaped %d\n", pid);
    if(++exited.val >= started.max){
      printf("OSS: All children exited at system time %lu:%li\n", data->timer.tv_sec, data->timer.tv_usec);
      loop_flag = 0;
    }
  }
}

static void deny_all(){
  int i;
  sigset_t mask, oldmask;
  struct msgbuf mbuf;

  //Informing all to stop
  sigemptyset(&mask);
  sigaddset(&mask, SIGCHLD);
  sigprocmask(SIG_BLOCK, &mask, &oldmask);

  //Denying all the blocked reference requests
  for(i=0; i < RUNNING_MAX; i++){
    if(data->users[i].pid == 0){
      continue;
    }

    critical_lock();
    if(data->users[i].ref.state != REF_DENY){

      data->users[i].ref.state = REF_DENY;

      mbuf.mtype = data->users[i].pid;
      mbuf.user_id = -1; //user will quit on id=-1
      if(msgsnd(qid, &mbuf, MSGBUF_SIZE, 0) == -1){
        perror("msgsnd");
      }
    }
    critical_unlock();
  }

  sigprocmask(SIG_SETMASK, &oldmask, NULL);
}

//Exiting master cleanly
static void clean_exit(const int code){

  if(data){

    critical_lock();
    data->stop = 1;
    critical_unlock();

//sending empty slice to all running processes for termination
    while(running.val > 0){
//some user may be waiting on timer
      data->timer.tv_sec++;
      deny_all();
      reap_zombies();
    }

  }

  printf("Master:  Done (exit %d) at system time %lu:%li\n", code, data->timer.tv_sec, data->timer.tv_usec);

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
  char user_id[10], user_scheme[10];

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
    snprintf(user_scheme, sizeof(user_scheme), "%d", addr_scheme.val);

    execl("user", "user", user_id, user_scheme, NULL);
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
  printf("\t-m 0|1\t Address scheme - random or WT\n");
}

//Set options specified as program arguments
static int set_options(const int argc, char * const argv[]){

  //these are the default options used as in the previous Projects
  started.val = 0; started.max = 100; //max users started
  exited.val  = 0; exited.max = 100;
  runtime.val = 0; runtime.max = 2;  //maximum runtime in real seconds
  running.val = 0; running.max = RUNNING_MAX;
  addr_scheme.val = 0; addr_scheme.max = 0;

  int c, redir=0;
	while((c = getopt(argc, argv, "hc:l:t:m:v")) != -1){
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
      case 'm': addr_scheme.val = atoi(optarg); break;
			default:
				printf("Error: Option '%c' is invalid\n", c);
				return -1;
		}
	}

  if(redir == 0){
    stdout = freopen("log.txt", "w", stdout);
  }

  if((addr_scheme.val < 0) || (addr_scheme.val > 1)){
    return -1;
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
      printf("Master: Signal INT receivedat system time %lu:%li\n", data->timer.tv_sec, data->timer.tv_usec);
      loop_flag = 0;  //stop master loop
      break;

    case SIGALRM: //creates an alarm - for end of runtime
      printf("Master:  Signal ALRM received at system time %lu:%li\n", data->timer.tv_sec, data->timer.tv_usec);
      loop_flag = 0;
      data->stop = 1;
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

  int i, num_ready = 0;
  for(i=0; i < blocked.len; i++){

    const int id = blocked.items[i];
    struct user_pcb * usr = &data->users[id];

    if(timercmp(&data->timer, &usr->ref.loadt, >)){
      num_ready++;

      q_deq(&blocked, i);

      const int pidx = PAGE_IDX(usr->ref.vaddr);
      struct virt_page * vp = &usr->PT[pidx];

      eviction_update_timestamp(data->FT, vp->f);
//when process becomes ready
      usr->state = ST_READY;
      usr->ref.state = REF_ACCEPT;  //change request state to precess

      mstat.num_req++;
      printf("OSS: Unblocked process with PID %d waiting on addres %d at system time %lu:%li\n",
        usr->id, usr->ref.vaddr, data->timer.tv_sec, data->timer.tv_usec);

      //unblocking the user
      mbuf.mtype = usr->pid;  //set its pid as type
      mbuf.user_id = usr->id;
      if(msgsnd(qid, (void*)&mbuf, MSGBUF_SIZE, 0) == -1){
        perror("msgrcv");
        return -1;
      }
    }
  }

  return num_ready;
}

static int mem_addr_fault(struct user_pcb * usr){

	mstat.num_fault++;

	//to get the page index
  const int pidx = PAGE_IDX(usr->ref.vaddr);

	//to get a free frame index
  int f = find_frame();
  if(f >= 0){
		printf("Master: Using free frame %d for P%d page %d at system time %lu:%li\n",
      f, usr->id, pidx, data->timer.tv_sec, data->timer.tv_usec);

  }else{	//when there is no frame which is free

		mstat.num_evict++;
	  f = eviction_policy(data->FT);

	  struct frame * fr = &data->FT[f];
	  struct virt_page * vp = &data->users[fr->u].PT[fr->p];
	//evicting page
	  printf("Master: Evicting page %d of P%d at system time %li:%lu\n",
	      fr->p, fr->u, data->timer.tv_sec, data->timer.tv_usec);

	  vp->ref = 0;
	// clearing and swapping
	  printf("Master: Clearing frame %d and swapping in P%d page %d at system time %li:%lu\n",
	    vp->f, usr->id, pidx, data->timer.tv_sec, data->timer.tv_usec);

		if(fr->state != FR_DIRTY){
      struct timeval tv;
      tv.tv_sec = 0; tv.tv_usec = 14;
      timerinc(&data->timer, &tv);
	//Dirty Bit setting and adding additionla time to the clock
			printf("Master: Dirty bit of frame %d is set, adding additional time to clock at system time %li:%lu\n",
				vp->f, data->timer.tv_sec, data->timer.tv_usec);
		}

	  f = vp->f;
	  frame_reset(&data->FT[vp->f], vp->f);
	  vp->f = -1;
	}

  return f;
}

static int mem_page_load(struct user_pcb * usr){
  int rv;
  struct timeval tv;

  const int pidx = PAGE_IDX(usr->ref.vaddr);

	struct virt_page * vp = &usr->PT[pidx];

	//if page has no frame (i.e., not in memory)
  if(vp->f < 0){

    printf("Master: Address %d is not in a frame, pagefault at system time %li:%lu\n",
      usr->ref.vaddr, data->timer.tv_sec, data->timer.tv_usec);

		//for saving frame index in page
  	vp->f = mem_addr_fault(usr);
  	vp->ref = 1;

		//for marking frame as used, and save page/user details
    frame_mark_used(&data->FT[vp->f], vp->f);
  	data->FT[vp->f].p = pidx;
  	data->FT[vp->f].u = usr - data->users;

    //14 ms load time for the device
    tv.tv_sec = 0; tv.tv_usec = 14;
    usr->ref.loadt = data->timer;
    timerinc(&usr->ref.loadt, &tv);

		//adding to the blocked queue
    const int user_id = usr - data->users;
    q_enq(&blocked, user_id);

		//won't send reply yet
  	rv = REF_BLOCK;

  }else{
    tv.tv_sec = 0; tv.tv_usec = 10;
    timerinc(&data->timer, &tv);

    eviction_update_timestamp(data->FT, vp->f);
    rv = REF_ACCEPT;
  }

  return rv;
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
  //printf("OSS: Acknowledged process with PID %u is making a reference for address %d at system time %lu:%li\n",
  //  usr->id, usr->ref.vaddr, data->timer.tv_sec, data->timer.tv_usec);

  //processing the request
  int reply = 0;
  if(usr->ref.req == MEM_READ){ //if user wants to MEM_READ

    mstat.num_rd++;
    printf("Master: P%d making read reference on address %d at system time %li:%lu\n",
      usr->id, usr->ref.vaddr, data->timer.tv_sec, data->timer.tv_usec);

    usr->ref.state = mem_page_load(usr);
    if(usr->ref.state == REF_ACCEPT){
      const int pidx = PAGE_IDX(usr->ref.vaddr);
      struct virt_page * vp = &usr->PT[pidx];

      printf("Master: Address %d in frame %d, giving data to P%d at system time %li:%lu\n", usr->ref.vaddr, vp->f, usr->id, data->timer.tv_sec, data->timer.tv_usec);

      reply = 1;
    }else if(usr->ref.state == REF_DENY){ //reference for address is denied
      printf("Master: P%d refrence for address %d was denied %lu:%lu\n", usr->id, usr->ref.vaddr, data->timer.tv_sec, data->timer.tv_usec);
      reply = 1;
    }

  }else if(usr->ref.req == MEM_WRITE){ //if user wants to MEM_WRITE
    mstat.num_wr++;

    printf("Master: P%d making write reference on address %d at system time %li:%lu\n",
      usr->id, usr->ref.vaddr, data->timer.tv_sec, data->timer.tv_usec);

    usr->ref.state = mem_page_load(usr);
    if(usr->ref.state == REF_ACCEPT){
      const int pidx = PAGE_IDX(usr->ref.vaddr);
      struct virt_page * vp = &usr->PT[pidx];

      printf("Master: Address %d in frame %d, writing data to frame at system time %li:%lu\n",
        usr->ref.vaddr, vp->f, data->timer.tv_sec, data->timer.tv_usec);

      struct frame * fr = &data->FT[vp->f];
      fr->state = FR_DIRTY;	//we have written to frame, marking it as dirty

      reply = 1;
    }else if(usr->ref.state == REF_DENY){
      printf("Master: P%d refrence for address %d was denied at system time %li:%lu\n", usr->id, usr->ref.vaddr, data->timer.tv_sec, data->timer.tv_usec);
      reply = 1;
    }
  }else if(usr->ref.req == MEM_TERM){
    printf("Master: P%d has terminated at system time %li:%lu\n", usr->id, data->timer.tv_sec, data->timer.tv_usec);
    usr->ref.state = REF_ACCEPT;
  }

  //if we should unblock the user
  if(reply){

    mstat.num_req++;

    mbuf.mtype = usr->pid;  //setting its pid as type
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
  mstat.num_lines++;

  return 0;
}

static void print_memmap(){
  int i;

  printf("Currently available memory at time %li.%li\n", data->timer.tv_sec, data->timer.tv_usec);
	mstat.num_lines++;

  printf("\t\t\tOccupied\tDirtyBit\ttimeStamp\n");
  for(i=0; i < FTBL_SIZE; i++){
    const struct frame * fr = &data->FT[i];
    if(fr->p >= 0){
      printf("Frame %2d\t\tYes\t\t%7d\t%8d\n", i, (fr->state == FR_DIRTY) ? 1 : 0, fr->lru);
    }else{
      printf("Frame %2d\t\tNo\t\t%7d\t%8d\n", i, 0, fr->lru);
    }
  }
  printf("\n");
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

  alarm(runtime.max);

  bzero(&mstat, sizeof(struct mstat));

  q_init(&blocked, started.max);  //initializing the queue
  //allocating and setting memory
  mem_alloc();

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

    if((verbose == 1) && ((mstat.num_req % 500) == 1)){
      print_memmap();
      fflush(stdout);
    }
	}

	//For printing the statistics at the end
  printf("Time taken: %lu:%li\n", data->timer.tv_sec, data->timer.tv_usec);

  printf("Number of references: %d\n", mstat.num_req);
	printf("Total Number of reads: %d\n", mstat.num_rd);
	printf("Total Number of writes: %d\n", mstat.num_wr);

	printf("Total Number of evictions: %d\n", mstat.num_evict);
	printf("Total Number of faults: %d\n", mstat.num_fault);

	printf("Memory access / second: %.2f\n", (float) mstat.num_req / data->timer.tv_sec);
	printf("Faults / reference: %.2f\n", (float)mstat.num_fault / mstat.num_req);

	clean_exit(EXIT_SUCCESS);
  return 0;
}
