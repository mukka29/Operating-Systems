//header files
#include <sys/time.h>
#include "res.h"

#define RUNNING_MAX 18

//Constants for this program
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
	struct timeval timer; //time
	struct user_pcb users[RUNNING_MAX];

	struct resource R[RCOUNT]; // the resource descriptors
};

//for a user process to terminate, this is the Probability in percent
#define TERMINATE_PROBABILITY 15
#define RELEASE_PROBABILITY 30

//Message queue buffer
struct msgbuf {
	long mtype; //the type of message
	int user_id; //who is sending the message. Needed by master, to send reply
};

//showing how big is the message ( without the mtype)
#define MSGBUF_SIZE (sizeof(int) + sizeof(enum states))
