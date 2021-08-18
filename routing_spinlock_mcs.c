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

// 0, 3, 5, 6, 2, 4, 7, 1, 9, 14, 10, 13, 15, 8, 11, 12, 0
int idCov[16] = {3, 9, 4, 5, 7, 6, 2, 1, 11, 14, 13, 12, 0, 15, 10, 8};  // this array should be changed on different CPU
thread_local int routingID;
thread_local int cid;
volatile atomic_int GlobalLock = 0;
int zero = 0;
int num_of_vcore;
int num_of_thread;

typedef struct mcs_node {
	// mcs node contain atomic pointer point to next node
	_Atomic(struct mcs_node)* next;
	int routingID;
} mcs_node;
// type define a atomic mcs node for better access
typedef _Atomic(mcs_node) atomic_mcs_node;

atomic_mcs_node waitArray[16];
atomic_int hasLock[16];

int atomic_push_back(atomic_mcs_node* arr, int val) {
	// due to we are dealing with atomic struct
	// the access is different
	// we have to declarate a struct on stack memory
	// than we can atmoic store it back
	// current is the one on stack
	mcs_node current = atomic_load(arr);
	// tmp is the next node
	atomic_mcs_node* tmp = malloc(sizeof(mcs_node));
	atomic_init(tmp, ((mcs_node){.routingID = val, .next = NULL}));
	current.next = tmp;
	atomic_store(arr, current);
	return 1;
}

int atomic_pop_front(atomic_mcs_node* head) {
	// same as push_back function
	mcs_node current = atomic_load(head);
	int ret = current.routingID;
	atomic_store(head, *current.next);
	return ret;
}

void spin_init() {
	// init
	// still has the cid routingID issue discussed before
	// in the program use cid
	cid = sched_getcpu();
	routingID = idCov[cid];
	for (int i = 0; i < num_of_vcore; i++) {
		atomic_init((waitArray + i), ((mcs_node){.routingID = routingID, .next = NULL}));
		atomic_init((hasLock + i), 0);
	}
}

void spin_lock() {
	// push current thread in mcs list(waitarray)
	atomic_push_back(waitArray + cid, cid);
	// hasLock[cid] == 0 -> means don't have the lock
	// waitarray + cid != NULL -> this list is not empty
	while (!hasLock[cid] || waitArray + cid != NULL) {
		zero = 0;
		if (atomic_compare_exchange_strong(&GlobalLock, &zero, 1)) {
			printf("enter by first\n");
			atomic_push_back(waitArray + cid, routingID);
			atomic_store((hasLock + cid), 1);
			return;
		}
	}
	atomic_push_back((waitArray + cid), routingID);
	atomic_store((hasLock + cid), 1);
	return;
}

void spin_unlock() {
	// if the routingID(the destination vcore) have waiting thread
	if (waitArray + routingID) {
		atomic_pop_front(waitArray + cid);
		atomic_store(hasLock + cid, 0);
		atomic_store(hasLock + routingID, 1);
		printf("parse lock to %d\n", routingID);
	} else {
		atomic_store(&GlobalLock, 0);
		printf("unlock and reset\n");
	}
}

void thread() {
	cid = sched_getcpu();
	routingID = idCov[cid];
	// printf("cid = %d\nroutingID = %d\n", cid, routingID);

	spin_lock();
	printf("routingID : %d(cid : %d) has CS\n", routingID, cid);
	sleep(1);
	printf("routingID : %d(cid : %d) unlock\n", routingID, cid);
	spin_unlock();
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
