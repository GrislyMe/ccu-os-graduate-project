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

int counter = 0;

typedef struct mcs_node {
	// mcs node contain atomic pointer point to next node
	struct mcs_node* next;
	// only store on head
	// parse when pop
	struct mcs_node* tail;
	int lock;
	int routingID;
} mcs_node;

mcs_node waitArray[16];

int atomic_push_back(mcs_node* arr, int val) {
	// due to we are dealing with atomic struct
	// the access is different
	// we have to declarate a struct on stack memory
	// than we can atmoic store it back
	// current is the one on stack
	// tmp is the next node
	mcs_node* tmp = malloc(sizeof(mcs_node));
	if (arr->tail)
		arr = arr->tail;
	*tmp = (mcs_node){.routingID = val, .next = NULL, .lock = 0, .tail = NULL};
	arr->next = tmp;
	return 1;
}

int atomic_pop_front(mcs_node* head) {
	// same as push_back function
	if (head->next == NULL)
		return 0;
	mcs_node tmp = *head->next;
	tmp.tail = head->tail;
	*head = tmp;
	// return pop node value
	return 1;
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
	while ((waitArray + cid)->lock == 0) {
		zero = 0;
		if (atomic_compare_exchange_strong(&globalLock, &zero, 1)) {
			zero = 0;
			printf("enter by first\n");
			return;
		}
	}
	(waitArray + cid)->lock = 0;
	return;
}

void spin_unlock() {
	// if the routingID(the destination vcore) have waiting thread
	atomic_pop_front(waitArray + cid);
	if ((waitArray + routingID)->next != NULL) {
		printf("parse lock to %d\n", routingID);
		atomic_push_back(waitArray + routingID, routingID);
		(waitArray + routingID)->lock = 1;
	} else {
		// release the global lock
		printf("unlock and reset\n");
		atomic_store(&globalLock, 0);
	}
}

void thread(int threadID) {
	cid = sched_getcpu();
	routingID = idCov[cid];

	spin_lock();
	printf("cid: %d(routingID: %d) has CS\n", cid, routingID);
	counter++;
	printf("cid: %d(routingID: %d) unlock\n", cid, routingID);
	spin_unlock();
}

int main() {
	// init
	num_of_vcore = get_nprocs();
	num_of_thread = num_of_vcore * num_of_vcore * num_of_vcore;
	spin_init();

	// create and join thread
	pthread_t* tid = (pthread_t*)malloc(sizeof(pthread_t) * num_of_thread);

	for (int i = 0; i < num_of_thread; i++)
		pthread_create(&tid[i], NULL, (void*)thread, NULL);

	for (int i = 0; i < num_of_thread; i++)
		pthread_join(tid[i], NULL);

	printf("counter = %d\nexpect = %d\n", counter, num_of_vcore * num_of_vcore * num_of_vcore);
	return 0;
}
