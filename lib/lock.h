#include <stdatomic.h>
#include <stddef.h>
#include <time.h>

#define mcs_null (struct mcs_node*)NULL

typedef struct mcs_node {
	_Atomic typeof(struct mcs_node*) next;
	atomic_bool lock;
} mcs_node;

typedef struct info {
	int rs_size;
	int cid;
} info;

extern int rs_set[];
extern int rs_set_size;

struct timespec time_diff(struct timespec start, struct timespec end);

void spin_init();
void soa_spin_init(int num_of_vcore, int* tsp_order);

void spin_lock();

void spin_unlock();

void ticket_lock();

void ticket_unlock();
