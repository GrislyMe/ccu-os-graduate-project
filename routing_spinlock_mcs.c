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

int idCov[16] = {3, 9, 4, 5, 7, 6, 2, 1, 11, 14, 13, 12, 0, 15, 10, 8};  // this array should be changed on different CPU
thread_local int routingID;
thread_local int cid;
// track who has lock
volatile atomic_int globalLock;
int num_of_vcore;
int num_of_thread;

typedef struct mcs_node {
	// mcs node contain atomic pointer point to next node
	_Atomic(struct mcs_node)* next;
	// only store on head
	// parse when pop
	_Atomic(struct mcs_node)* tail;
	atomic_int lock;
	int routingID;
} mcs_node;
// type define a atomic mcs node for better access
typedef _Atomic(mcs_node) atomic_mcs_node;

atomic_mcs_node waitArray[16];

int atomic_push_back(atomic_mcs_node* arr, int val) {
	// due to we are dealing with atomic struct
	// the access is different
	// we have to declarate a struct on stack memory
	// than we can atmoic store it back
	// current is the one on stack
	mcs_node current = atomic_load(arr);
	// tmp is the next node
	atomic_mcs_node* tmp = malloc(sizeof(mcs_node));
	atomic_store(tmp, ((mcs_node){.routingID = val, .next = NULL}));
	current.next = tmp;
	atomic_store(arr, current);
	return 1;
}

int atomic_pop_front(atomic_mcs_node* head) {
	// same as push_back function
	mcs_node current = atomic_load(head);
	mcs_node tmp = *current.next;
	tmp.tail = current.tail;
	atomic_store(head, tmp);
	// return pop node value
	return current.routingID;
}

void spin_init() {
	// init
	// still has the cid routingID issue discussed before
	// in the program use cid
	routingID = idCov[cid];
	atomic_init(&globalLock, 0);
}

void spin_lock() {
	// push current thread in mcs list(waitarray)
	atomic_push_back(waitArray + cid, cid);
	int zero;
	// hasLock == -1 -> means no one owns the lock
	// waitarray + cid != NULL -> this waiting list is not empty
	mcs_node current = atomic_load(waitArray + cid);
	mcs_node tmp = current;
	tmp.lock = 1;
	while (atomic_compare_exchange_strong((waitArray + cid), &tmp, current)) {
		tmp = current;
		tmp.lock = 1;
		zero = 0;
		if (atomic_compare_exchange_strong(&globalLock, &zero, cid)) {
			printf("enter by first\n");
			return;
		}
	}
	return;
}

void spin_unlock() {
	// if the routingID(the destination vcore) have waiting thread
	atomic_pop_front(waitArray + cid);
	if (atomic_load(waitArray + routingID).next != NULL) {
		mcs_node dest = atomic_load(waitArray + routingID);
		dest.lock = 1;
		atomic_push_back(waitArray + routingID, routingID);
		atomic_store(waitArray + routingID, dest);
		printf("parse lock to %d\n", routingID);
	} else {
		// release the global lock
		globalLock = 0;
		printf("unlock and reset\n");
	}
}

void thread(int threadID) {
	cid = sched_getcpu();
	routingID = idCov[cid];
	// printf("cid = %d\nroutingID = %d\n", cid, routingID);

	spin_lock();
	printf("cid: %d(routingID: %d) has CS\n", cid, routingID);
	sleep(1);
	printf("cid: %d(routingID: %d) unlock\n", cid, routingID);
	spin_unlock();
}

int main() {
	// init
	num_of_vcore = get_nprocs();
	num_of_thread = num_of_vcore;
	spin_init();

	// create and join thread
	pthread_t* tid = (pthread_t*)malloc(sizeof(pthread_t) * num_of_thread);

	for (int i = 0; i < num_of_thread; i++)
		pthread_create(&tid[i], NULL, (void*)thread, NULL);

	for (int i = 0; i < num_of_thread; i++)
		pthread_join(tid[i], NULL);

	return 0;
}
