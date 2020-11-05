#include <string.h>
#include <sys/shm.h>
#include <sys/msg.h>
#include <sys/types.h>
#include <sys/time.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "data.h"

static int mid=-1, qid = -1;	//memory and msg queue identifiers
static struct data *data = NULL;	//shared memory pointer

//Attach the shared memory pointer
static int attach_data(){

	key_t k1 = ftok(PATH_KEY, MEM_KEY);
	key_t k2 = ftok(PATH_KEY, MSG_KEY);
	if((k1 == -1) || (k2 == -1)){
		perror("ftok");
		return -1;
	}

	//get shared memory id
	mid = shmget(k1, 0, 0);
  if(mid == -1){
  	perror("semget");
  	return -1;
  }

	//get message queue ud
	qid = msgget(k2, 0);
  if(qid == -1){
  	perror("semget");
  	return -1;
  }

	//attach to the shared memory
	data = (struct data *) shmat(mid, NULL, 0);
	if(data == NULL){
		perror("shmat");
		return -1;
	}
	return 0;
}

//Lock the critical section
static int critical_lock(){
	struct msgbuf mbuf;	//message buffer

	//fprintf(stderr, "[USER|%li:%i] %d LOCKING critical section\n",
	//	data->timer.tv_sec, data->timer.tv_usec, getpid());

	//request from master access to critical section.
	mbuf.mtype = MSG_ENTER;
	mbuf.pid = getpid();
	if(msgsnd(qid, &mbuf, MSGBUF_SIZE, 0) == -1){
		perror("msgsnd");
		return -1;
	}

	//It will block, until section is available
	if(msgrcv(qid, (void*)&mbuf, MSGBUF_SIZE, mbuf.pid, 0) == -1){
		perror("user:msgrcv");
		return -1;
	}

	//fprintf(stderr, "[USER|%li:%i] %d LOCKED critical section\n",
	//	data->timer.tv_sec, data->timer.tv_usec, getpid());

	return 0;
}

//Unlock critical section
static int critical_unlock(){
	struct msgbuf mbuf;

	//tell master we are leaving the critical section
	mbuf.mtype = MSG_LEAVE;
	mbuf.pid = getpid();
	if(msgsnd(qid, &mbuf, MSGBUF_SIZE, 0) == -1){
		perror("msgsnd");
		return -1;
	}

	//wait for reply
	if(msgrcv(qid, (void*)&mbuf, MSGBUF_SIZE, mbuf.pid, 0) == -1){
		perror("user:msgrcv");
		return -1;
	}

	//fprintf(stderr, "[USER|%li:%i] %d UNLOCKED critical section\n",
	//	data->timer.tv_sec, data->timer.tv_usec, getpid());

	return 0;
}

//Inform master we are terminating
static int critical_exit(){
	struct msgbuf mbuf;

	mbuf.mtype = MSG_EXIT;
	mbuf.pid = getpid();
	if(msgsnd(qid, &mbuf, MSGBUF_SIZE, 0) == -1){
		perror("msgsnd");
		return -1;
	}

	//wait for reply
	if(msgrcv(qid, (void*)&mbuf, MSGBUF_SIZE, mbuf.pid, 0) == -1){
		perror("user:msgrcv");
		return -1;
	}

	//fprintf(stderr, "[USER|%li:%i] %d EXIT critical section\n",
	//	data->timer.tv_sec, data->timer.tv_usec, getpid());

	return 0;
}

int main(const int argc, char * const argv[]){

	if(	attach_data() < 0)
		return EXIT_FAILURE;

	srand(time(NULL));

	struct timeval inc, end;
	//determine how long we will run
	inc.tv_sec = 0;
	inc.tv_usec = rand() % 1000000;	//max is 1M usec

	//copy shared timer to end and sub with increment
	critical_lock();
	timeradd(&inc, &data->timer, &end);
	critical_unlock();

	//fprintf(stderr, "[USER|%li:%i] User %d running until %d.%d\n",
	//	data->timer.tv_sec, data->timer.tv_usec, getpid(), end.tv_sec, end.tv_usec);

	int stop = 0;
	while(!stop){

		if(critical_lock() < 0){
			break;
		}

		if(timercmp(&data->timer, &end, >)){
			if(data->user_pid == 0){	//if pid is not set
				data->user_pid = getpid();	//save our own pid
				//fprintf(stderr, "[USER|%li:%i] User %d saved pid\n", data->timer.tv_sec, data->timer.tv_usec, getpid());
				stop = 1;	//stop the master loop
			}
		}
		if(critical_unlock() < 0){
			break;
		}
	}

	critical_exit();
	shmdt(data);

	return EXIT_SUCCESS;
}
