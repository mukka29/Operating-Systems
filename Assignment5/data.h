#include <sys/time.h>
#include "res.h"

#define RUNNING_MAX 18

//Constants for our program
#define PATH_KEY "/"
#define MSG_KEY 1111
#define MEM_KEY 2222
#define SEM_KEY 3333

enum states { ST_READY=1, ST_BLOCKED, ST_TERM, ST_COUNT};

struct user_pcb {
	int	pid;
	int id;
	enum states state;
	struct request request;

	struct resource r[RCOUNT];
};

struct data {
	struct timeval timer;	//time
	struct user_pcb users[RUNNING_MAX];

	struct resource R[RCOUNT];	/* resource descriptors */
};

//Probability in percent, for a user process to terminate
#define TERMINATE_PROBABILITY 15
#define RELEASE_PROBABILITY 30

//Message queue buffer
struct msgbuf {
	long mtype;					//the type of message
	int user_id;						//who is sending the message. Needed by master, to send reply
};

//how big is our message ( without the mtype)
#define MSGBUF_SIZE (sizeof(int) + sizeof(enum states))
