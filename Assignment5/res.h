//the count of resources
#define RCOUNT	20
//the maximum value of a resource
#define R_MAX 10

//this states a request can be in
enum request_state { ACCEPTED=0, BLOCKED, WAITING, DENIED};

struct request {
	int id;
	int val;
	int state;
};

struct resource {
	int total;
	int available;
	int is_shared;
};

void res_alloc(struct resource r[RCOUNT]);
void res_alloc_total(struct resource r[RCOUNT], struct resource R[RCOUNT]);

int res_rand_request(const struct resource r[RCOUNT]);
int res_rand_release(const struct resource r[RCOUNT]);
