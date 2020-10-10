
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/shm.h>
#include <unistd.h>
#include <sys/sem.h>
#include <sys/types.h>
#include "data.h"

static int mid=-1, sid = -1;	//memory and the semaphore identifiers
static struct data *data = NULL;	//shared memory pointer
static int wi = 0;	//our word index

//Attaching the shared memory pointer
static int attach_data(){

	key_t k1 = ftok(PATH_KEY, MEM_KEY);
	key_t k2 = ftok(PATH_KEY, SEM_KEY);
	if((k1 == -1) || (k2 == -1)){
		perror("ftok");
		return -1;
	}

	mid = shmget(k1, 0, 0);
  if(mid == -1){
  	perror("semget");
  	return -1;
  }

	sid = semget(k2, 2, 0);
  if(sid == -1){
  	perror("semget");
  	return -1;
  }

	data = (struct data *) shmat(mid, NULL, 0);
	if(data == NULL){
		perror("shmat");
		return -1;
	}
	return 0;
}

//Setting the word index option
static int set_options(const int argc, char * const argv[]){
	if(argc != 2){
		fprintf(stderr, "Usage: palin <word index>\n");
		return -1;
	}

	wi = atoi(argv[1]);
	if(wi < 0){
		fprintf(stderr, "Error: Word index is invalud\n");
		return -1;
	}

	return 0;
}

//Checking if word is a palindrome
static int palindrome_word(const char * myword){
	int i;
	const int len = strlen(myword) - 1;
	const int middle = len / 2;

	//comparing the two halfs of string
	for(i=0; i < middle; i++)
		if(myword[i] != myword[len - i - 1]){
			return 0;
		}

	return 1;	//if the word is palindrome
}

// sem_num - 0 is for palin.out, 1 is for nopalin.out
static int critical_lock(const int sem_num){
	struct sembuf sop;

	fprintf(stderr, "[PALIN|%li:%i] %d LOCKING critical section %i\n",
		data->timer.tv_sec, data->timer.tv_usec, getpid(), sem_num);

	sop.sem_num = sem_num;
	sop.sem_flg = 0;
	sop.sem_op  = -1;
	if(semop(sid, &sop, 1) == -1) {
		 perror("semop");
		 return -1;
	}
	fprintf(stderr, "[PALIN|%li:%i] %d LOCKED critical section %d\n",
		data->timer.tv_sec, data->timer.tv_usec, getpid(), sem_num);

	return 0;
}

//Unlocking the critical section
static int critical_unlock(const int sem_num){
	struct sembuf sop;

	sleep(2);

	sop.sem_num = sem_num;
	sop.sem_flg = 0;
	sop.sem_op  = 1;
	if(semop(sid, &sop, 1) == -1) {
		 perror("semop");
		 return -1;
	}

	fprintf(stderr, "[PALIN|%li:%i] %d UNLOCKED critical section %d\n",
		data->timer.tv_sec, data->timer.tv_usec, getpid(), sem_num);

	return 0;
}

int main(const int argc, char * const argv[]){

	if(	(set_options(argc, argv) < 0) ||
			(attach_data() < 0))
		return EXIT_FAILURE;

	srand(time(NULL));

	//taking pointer to our word
	const char * myword = data->mylist[wi];
	const int palindrome = palindrome_word(myword);
	const int sem_num = (palindrome) ? 1 : 0;

	sleep(rand() % 3);

	critical_lock(sem_num);

	FILE * fp = (palindrome) ? fopen("palin.out", "a")
													 : fopen("nopalin.out", "a");

	if(fp == NULL){
		perror("fopen");
	}else{
		sleep(2);
		fprintf(fp, "%s", myword);
		fclose(fp);
	}

	critical_unlock(sem_num);


	printf("[PALIN|%li:%i] Palin %d finished\n", data->timer.tv_sec, data->timer.tv_usec, getpid());
	shmdt(data);

	return EXIT_SUCCESS;
}
