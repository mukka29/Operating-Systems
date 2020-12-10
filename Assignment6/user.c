//header files
#include <string.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <sys/sem.h>
#include <sys/types.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <time.h>
#include <math.h>

#include "data.h"

static int mid=-1, qid = -1, sid = -1;	//identifiers for memory and message queue
static struct data *data = NULL; //creating pointer for shared memory
static struct user_pcb * user = NULL;
static unsigned int user_id = -1;
static unsigned int user_scheme = 0;	//-m option at master for memory address generation

static void update_weights(){
	int i;

	for(i=1; i < PTBL_SIZE; i++){
		user->WT[i] += user->WT[i-1];
	}
}

//Attaching the shared memory pointer
static int attach_data(const int argc, char * const argv[]){
	int n;
	key_t k1 = ftok(PATH_KEY, MEM_KEY);
	key_t k2 = ftok(PATH_KEY, MSG_KEY);
	key_t k3 = ftok(PATH_KEY, SEM_KEY);
	if((k1 == -1) || (k2 == -1) || (k3 == -1)){
		perror("ftok");
		return -1;
	}

	//to get shared memory id
	mid = shmget(k1, 0, 0);
  if(mid == -1){
  	perror("shmget");
  	return -1;
  }

	//to get message queue ud
	qid = msgget(k2, 0);
  if(qid == -1){
  	perror("msgget");
  	return -1;
  }

	//to get shared memory id
	sid = semget(k3, 0, 0);
  if(sid == -1){
  	perror("semget");
  	return -1;
  }

	//attaching to the shared memory
	data = (struct data *) shmat(mid, NULL, 0);
	if(data == NULL){
		perror("shmat");
		return -1;
	}

	user_id = atoi(argv[1]);
	user_scheme = atoi(argv[2]);
  user = &data->users[user_id];


	for(n=0; n < PTBL_SIZE; n++){
		user->WT[n] = 1.0 / (n+1);
	}
	return 0;
}

static int critical_lock(){
	struct sembuf sop;

	sop.sem_num = 0;
	sop.sem_flg = 0;
	sop.sem_op  = -1;
	if(semop(sid, &sop, 1) == -1) {
		 perror("semop");
		 return -1;
	}
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

static int gen_rand_addr(){

	const int page = rand() % PTBL_SIZE;
	const int offset = rand() % PAGE_KB;
	const int vaddr = (page*PAGE_KB) + offset;

	return vaddr;
}

static int gen_weighted_addr(){
	int i;

	const double last_w = user->WT[PTBL_SIZE-1];
	const double rand_w = drand48() * last_w;
	const double w = fmod(rand_w, last_w);
	//printf("last=%lf, rand=%lf, w=%lf\n", last_w, rand_w, w);
	for(i=0; i < PTBL_SIZE; i++){
		if(w < user->WT[i]){
			break;
		}
	}
	update_weights();

	const int page = i;
	const int offset = rand() % PAGE_KB;
	const int vaddr = (page*PAGE_KB) + offset;
	return vaddr;
}

static int execute(){

	struct msgbuf mbuf;
	static int max_prob = 100;
	int decision = 0, r = (rand() % max_prob);

	//decide between request(0), relese(1) and termniate(2)
	//if(r >= TERMINATE_PROBABILITY){
		//decide between MEM_READ(0), MEM_WRITE(1)
		decision = (r < READ_PROBABILITY) ? MEM_READ /*MEM_READ*/ : MEM_WRITE /* MEM_WRITE */;
	//}else{
	//	return 1;	//stop the master loop
	//}

	//setting up the reference
	critical_lock();
	user->ref.req = decision;
	user->ref.vaddr = (user_scheme == 0) ? gen_rand_addr() : gen_weighted_addr();
	user->ref.state = REF_WAIT;
	critical_unlock();

	//sending our request
	mbuf.mtype = getppid();
	mbuf.user_id = user_id;
	if(msgsnd(qid, &mbuf, MSGBUF_SIZE, 0) == -1){
		perror("msgsnd");
		return 1;	//stop the master loop
	}

	//waiting for a reply from oss
	if(msgrcv(qid, (void*)&mbuf, MSGBUF_SIZE, getpid(), 0) == -1){
		perror("msgrcv");
		return 1;	//stop the master loop
	}

	if(mbuf.user_id == -1){
		return 1;	//stop the master loop
	}

	return 0;
}

static int execute_final(){

	struct msgbuf mbuf;

	critical_lock();
	user->ref.req = MEM_TERM;
	user->ref.vaddr = 0;
	user->ref.state = REF_WAIT;
	critical_unlock();

	//sending our request
	mbuf.mtype = getppid();
	mbuf.user_id = user_id;
	if(msgsnd(qid, &mbuf, MSGBUF_SIZE, 0) == -1){
		perror("msgsnd");
		return -1;	//stop master loop
	}

	return 0;
}

int main(const int argc, char * const argv[]){
	int ref_count = 0;
	if(attach_data(argc, argv) < 0)
		return EXIT_FAILURE;

	srand(getpid());
	srand48(getpid());

	int stop = 0;
	while(!stop){

		//checking for stop on every 1000 references
		if(++ref_count > 1000){
			if(critical_lock() < 0){
				break;
			}

			if(data->stop == 1){	//in case if master wants us to stop
				stop = 1;
				critical_unlock();
				break;
			}

			if(critical_unlock() < 0){
				break;
			}
			ref_count = 0;
		}

		//resource logic
		stop = execute();
	}

	execute_final();

	shmdt(data);
	return EXIT_SUCCESS;
}
