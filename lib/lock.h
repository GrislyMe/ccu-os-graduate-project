#include <stdatomic.h>
#include <stddef.h>

#define mcs_null (struct mcs_node*)NULL

typedef struct mcs_node {
	_Atomic typeof(struct mcs_node*) next;
	atomic_bool lock;
} mcs_node;

extern atomic_int lock;
extern atomic_ulong gtk;
extern unsigned long srv;
extern _Atomic struct mcs_node* tail;

void spin_init();

void spin_lock();

void spin_unlock();

void ticket_lock();

void ticket_unlock();
