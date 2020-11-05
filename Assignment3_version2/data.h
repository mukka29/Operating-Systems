#include <time.h>

//Constants used for this program
#define PATH_KEY "/"
#define MSG_KEY 1111
#define MEM_KEY 2222

struct data {
	struct timeval timer;	// represents the time
	int user_pid;	//user PID which is exiting
};

enum queue_msg {
	MSG_ALL=0,	//accepting message from everybody i.e., message all
	MSG_ENTER,	//used when a process wants to enter critical section i.e., message enter
	MSG_LEAVE,	//when a process wants to leave critical section i.e., message leave
	MSG_EXIT	//when a process wants to terminate i.e., message exit
};

//Buffer for message queue
struct msgbuf {
	long mtype;	//the type of message
	int pid;		//who is sending the message. Needed by master, to send reply
};

//shows how big is the message, i.e., our message buffer size (without the mtype)
#define MSGBUF_SIZE sizeof(int)
