#include <time.h>

//Constants for our program
#define WORD_MAX 100
#define LIST_MAX 100
#define PATH_KEY "/"
#define SEM_KEY 1111
#define MEM_KEY 2222

struct data {
	struct timeval timer;	//time
	char mylist[LIST_MAX][WORD_MAX];	//strings
};
