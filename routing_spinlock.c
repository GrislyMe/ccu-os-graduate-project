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
volatile atomic_int waitArray[16];
volatile atomic_int GlobalLock = 0;
int zero = 0;
int num_of_vcore;
int num_of_thread;

void spin_init() {
	routingID = idCov[sched_getcpu()];
	for (int i = 0; i < num_of_vcore; i++) {
		atomic_init(waitArray + i, 0);
	}
}

void spin_lock() {
	waitArray[routingID] = 1;
	while (atomic_load(waitArray + routingID) == 1) {
		zero = 0;
		if (atomic_compare_exchange_strong(&GlobalLock, &zero, 1)) {
			atomic_store(waitArray + routingID, 0);
			return;
		}
	}
	atomic_store(waitArray + routingID, 0);
	return;
}

void spin_unlock() {
	// the digit 6 is my core number
	// it should be changed on different CPU
	for (int i = 1; i < num_of_vcore; i++) {
		if (waitArray[(i + routingID) % 16] == 1) {
			atomic_store(waitArray + (i + routingID) % 16, 0);
			return;
		}
	}
	GlobalLock = 0;
}

void thread() {
	routingID = idCov[sched_getcpu()];

	spin_lock();
	printf("routingID : %d has CS\n", routingID);
	sleep(1);
	spin_unlock();
	printf("routingID : %d unlock\n", routingID);
}

int main() {
	// init
	num_of_vcore = get_nprocs();
	num_of_thread = num_of_vcore;
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
