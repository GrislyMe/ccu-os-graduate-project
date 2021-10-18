#define _GNU_SOURCE
#include <assert.h>
#include <math.h>
#include <pthread.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/syscall.h>
#include <sys/sysinfo.h>
#include <threads.h>
#include <unistd.h>

// 0 -> 4 -> 5 -> 3 -> 2 -> 1 -> 0
int idCov[6] = {4, 0, 1, 2, 5, 3};  // this array should be changed on different CPU
thread_local int routingID;
thread_local int cid;
volatile atomic_int GlobalLock = 0;
static int zero = 0;
int num_of_vcore;
int num_of_thread;

/*
 * Start to debug
 * The bug maybe is caused by the pointer assignment
 */

// the structure of my mcs_lock_node
typedef struct mcs_lock_node {
	volatile int waiting;
	struct mcs_lock_node* next;
	struct mcs_lock_node* tail;
	struct mcs_lock_node* head;
	int cid;
	int routingID;
} mcs_lock_node;

_Atomic volatile mcs_lock_node waitArray[6];

void thread_node_init(mcs_lock_node* mcs_node) {
	mcs_node->next = NULL;
	mcs_node->waiting = 1;
	mcs_node->cid = sched_getcpu();
	mcs_node->routingID = idCov[mcs_node->cid];
}

void spin_init() {
	routingID = idCov[sched_getcpu()];
	for (int i = 0; i < num_of_vcore; i++) {
		waitArray[i].next = NULL;
		waitArray[i].head = NULL;
		waitArray[i].tail = NULL;
		waitArray[i].waiting = 0;
		waitArray[i].cid = -1;
		waitArray[i].routingID = -1;
	}
}

void atomic_append(mcs_lock_node* mcs_node) {
	// atomic insert into the waitArray[mcs_node->cid]

	// If there is no thread waiting in the waitArray[coreOfThread]
	// Then assign the current mcs_node to this waitArray's head and tail
	// Otherwise, we assign the current mcs_node to the next of tail
	if (atomic_load_explicit(&waitArray[mcs_node->cid], memory_order_acquire).head == NULL) {
		atomic_store_explicit(&waitArray[mcs_node->cid].waiting, 1, memory_order_relaxed);
		atomic_store_explicit(&waitArray[mcs_node->cid].head, mcs_node, memory_order_relaxed);
		atomic_store_explicit(&waitArray[mcs_node->cid].tail, mcs_node, memory_order_release);
	} else {
		atomic_store_explicit(&waitArray[mcs_node->cid].tail->next, mcs_node, memory_order_relaxed);
		atomic_store_explicit(&waitArray[mcs_node->cid].tail, mcs_node, memory_order_release);
	}
}

void spin_lock(mcs_lock_node* mcs_node) {
	// atomic_append(mcs_node, SoA_array[routingID]);
	atomic_append(mcs_node);

	while (1) {
		zero = 0;  // let the variable "zero" always contain the value '0'
		if (mcs_node->waiting == 0)
			return;
		if (atomic_compare_exchange_strong(&GlobalLock, &zero, 1)) {
			mcs_node->waiting = 0;
			return;
		}
	}
}

void spin_unlock(mcs_lock_node* mcs_node) {
	// replace the current waitArray's head to its next one
	// if there is no more thread in the core
	// set the "waiting" value to zero
	waitArray[mcs_node->cid].head = waitArray[mcs_node->cid].head->next;
	if (waitArray[mcs_node->cid].head == NULL) {
		waitArray[mcs_node->cid].tail = NULL;
		waitArray[mcs_node->cid].waiting = 0;
	}

	for (int i = 0; i < num_of_vcore - 1; i++) {
		if (waitArray[(i + mcs_node->routingID) % num_of_vcore].waiting == 1) {
			if (waitArray[(i + mcs_node->routingID) % num_of_vcore].head != NULL) {
				// let the next thread entering the CS
				waitArray[(i + mcs_node->routingID) % num_of_vcore].head->waiting = 0;
			}
			return;
		}
	}
	GlobalLock = 0;
}

void thread() {
	// set all the information that will be used later
	mcs_lock_node* mcs_node = (mcs_lock_node*)malloc(sizeof(mcs_lock_node));
	thread_node_init(mcs_node);

	spin_lock(mcs_node);  // lock

	// CS
	printf("routingID : %d(cid : %d) has CS, waiting:%d\n", mcs_node->routingID, mcs_node->cid, mcs_node->waiting);
	sleep(0);
	printf("routingID : %d(cid : %d) unlock\n", mcs_node->routingID, mcs_node->cid);
	// CS

	spin_unlock(mcs_node);  // unlock
}

int main() {
	// init
	num_of_vcore = get_nprocs();
	num_of_thread = num_of_vcore * num_of_vcore;
	spin_init();

	// create and join thread
	pthread_t* tid = (pthread_t*)malloc(sizeof(pthread_t) * num_of_thread);

	// pthread_spin_lock(&lock);
	for (int i = 0; i < num_of_thread; i++) {
		pthread_create(&tid[i], NULL, (void*)thread, NULL);
	}
	// pthread_spin_unlock(&lock);

	for (int i = 0; i < num_of_thread; i++)
		pthread_join(tid[i], NULL);

	// pthread_spin_destroy(&lock);

	printf("global lock : %d\n", GlobalLock);
	return 0;
}
