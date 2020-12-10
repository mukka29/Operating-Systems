//header files
#include <sys/time.h>

#define RUNNING_MAX 18

//Constants for this program
#define PATH_KEY "/"
#define MSG_KEY 1111
#define MEM_KEY 2222
#define SEM_KEY 3333

enum states { ST_READY=1, ST_BLOCKED, ST_TERM, ST_COUNT};

#define PAGE_KB	1024 //size of the page table
#define PTBL_SIZE	32 //the amount of user PT
#define FTBL_SIZE 256 // the amount of FT


#define PAGE_IDX(vaddr) (vaddr / PAGE_KB)

//requests which the user sends to master
enum ref_request {MEM_READ=0, MEM_WRITE, MEM_TERM};
enum frame_state {FR_UNUSED=0, FR_DIRTY, FR_USED};

struct virt_page {
	int f;
	int ref; // for representing the reference bit
};

struct frame {	//mem_frame
	int p;	//for page
	int u;	//for user
	unsigned int lru;
	enum frame_state state;
};

enum request_state { REF_ACCEPT=0, REF_BLOCK, REF_WAIT, REF_DENY};
struct reference {
	int req; //MEM_READ or MEM_WRITE i.e., for Memory Read or Memory Write
	int vaddr;
	enum request_state state;
	struct timeval loadt;	//represnts time when will the reference be loaded
};

struct user_pcb {
	int	pid;
	int id;
	enum states state;
	struct reference ref;

	struct virt_page   PT[PTBL_SIZE];	//for page table
	double	WT[PTBL_SIZE];	// for weight table
};

struct data {
	struct timeval timer;	//time
	struct user_pcb users[RUNNING_MAX];

	int stop;	//the stop marker for users
  struct frame  FT[FTBL_SIZE];	//represents the frame table
};

//for a user process to terminate, this is the Probability in percent
#define TERMINATE_PROBABILITY 2
#define READ_PROBABILITY 60

//Message queue buffer
struct msgbuf {
	long mtype; //the type of message
	int user_id;//who is sending the message. Needed by master, to send reply
};

//showing how big is the message ( without the mtype)
#define MSGBUF_SIZE (sizeof(int))
