#include <time.h>

//Constants for our program
#define PATH_KEY "/"
#define MSG_KEY 1111
#define MEM_KEY 2222

struct data {
	struct timeval timer;	//time
	int user_pid;	//user PID that is exiting
};

enum queue_msg {
	MSG_ALL=0,	//accept message from everybody
	MSG_ENTER,	//when a process wants to enter critical section
	MSG_LEAVE,	//when a process wants to leave critical section
	MSG_EXIT		//when a process wants to terminate
};

//Message queue buffer
struct msgbuf {
	long mtype;	//the type of message
	int pid;		//who is sending the message. Needed by master, to send reply
};

//how big is our message ( without the mtype)
#define MSGBUF_SIZE sizeof(int)
