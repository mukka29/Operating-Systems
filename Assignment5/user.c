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

#include "data.h"

//100 ms maximum sleep time between user actions - [0; B]
#define B_MAX 100

static int mid=-1, qid = -1, sid = -1;	//identifiers for memory and message queue
static struct data *data = NULL; //creating pointer for shared memory
static struct user_pcb * user = NULL;
static unsigned int user_id = -1;

//Attaching the shared memory pointer
static int attach_data(const int argc, char * const argv[]){

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
  user = &data->users[user_id];

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

static int execute(){
	int rv = 0;
	struct msgbuf mbuf;
	static int max_prob = 100;
	int decision = 0, r = (rand() % max_prob);

	//deciding between request(0), relese(1) and termniate(2)
	if(r >= TERMINATE_PROBABILITY){
		//deciding between request(0), relese(1)
		decision = (r < RELEASE_PROBABILITY) ? 0 : 1;
	}else{
		//now only on option - terminate
		decision = 2;
	}

	switch(decision){
		case 0:		//release
			r = res_rand_release(user->r);
			if(r == -1){
				rv = 1;
			}
			break;

		case 1:	//request
			r = res_rand_request(user->r);
			if(r == -1){
				max_prob = RELEASE_PROBABILITY;
			}
			break;

		case 2:	//terminate
			user->request.id = -1;	//release all
			rv = 1;
			break;
	}

	if(r >= 0){

		//setting up the request
		critical_lock();
		user->request.id = r;
		if(decision == 0){
			user->request.val = -1 * user->r[r].available;
		}else{
			user->request.val = user->r[r].total;
		}
		user->request.state = WAITING;
		critical_unlock();

		//sending our request
		mbuf.mtype = getppid();
		mbuf.user_id = user_id;
		if(msgsnd(qid, &mbuf, MSGBUF_SIZE, 0) == -1){
			perror("msgsnd");
			return -1;
		}

		//waiting for reply from oss
		if(msgrcv(qid, (void*)&mbuf, MSGBUF_SIZE, getpid(), 0) == -1){
			perror("msgrcv");
			return -1;
		}

		if(mbuf.user_id == -1){
			rv = -1;	//stop the master loop
		}
	}
	return rv;
}

int main(const int argc, char * const argv[]){

	if(attach_data(argc, argv) < 0)
		return EXIT_FAILURE;

	srand(getpid());

	res_alloc_total(user->r, data->R);

	struct timeval inc, bend;
	//determining how long we will run
	inc.tv_sec = 0;
	inc.tv_usec = rand() % B_MAX;	//generate [0; B] period

	//copying shared timer to end and sub with increment
	critical_lock();
	timeradd(&inc, &data->timer, &bend);
	critical_unlock();

	int stop = 0;
	while(!stop){

		if(critical_lock() < 0){
			break;
		}

		//if its the time to request/release/terminate
		if(timercmp(&data->timer, &bend, >)){

			//regenerating bend
			inc.tv_usec = rand() % B_MAX;	//generate [0; B] period
			timeradd(&inc, &data->timer, &bend);

			if(critical_unlock() < 0){
				break;
			}

			//resource logic
			stop = execute();

		}else{
			if(critical_unlock() < 0){
				break;
			}
		}
	}

	//releasing all resources
	if(res_rand_release(user->r) != -1){
		struct msgbuf mbuf;

		critical_lock();
		//setting up the request
		user->request.id = -1;
		user->request.val = 0;
		user->request.state = WAITING;
		critical_unlock();

		//sending our request
		mbuf.mtype = getppid();
		mbuf.user_id = user_id;
		if(msgsnd(qid, &mbuf, MSGBUF_SIZE, 0) == -1){
			//perror("msgsnd");
			return -1;
		}

		//waiting for reply from oss
		if(msgrcv(qid, (void*)&mbuf, MSGBUF_SIZE, getpid(), 0) == -1){
			//perror("msgrcv");
			return -1;
		}
	}

	shmdt(data);
	return EXIT_SUCCESS;
}
