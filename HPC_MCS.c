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
// normal define
#define num_of_vcore 6

int num_of_thread;
int counter[100][num_of_vcore] = {0};
int globalData[300];
struct timespec thread_time[64];

// mcs define
typedef struct mcs_node {
	_Atomic struct mcs_node* next;
	atomic_bool lock;
} mcs_node;
_Atomic struct mcs_node* tail;
#define mcs_null (struct mcs_node*)NULL
void spin_lock(mcs_node* node) {
	atomic_init(&node->next, mcs_null);
	mcs_node* prev = atomic_exchange_explicit(&tail, node, memory_order_acq_rel);
	////assign prev as tail, if prev exist, go waiting.
	//
	if (prev != mcs_null) {
		atomic_init(&node->lock, true);
		atomic_store_explicit(&prev->next, node, memory_order_release);

		////waiting
		while (atomic_load_explicit(&node->lock, memory_order_acquire)) {
			// asm("pause");
			sleep(0);
		}
	}
}

void spin_unlock(mcs_node* node) {
	mcs_node* successor = atomic_load_explicit(&node->next, memory_order_acquire);
	// check successor
	if (successor == mcs_null) {
		mcs_node* prev = node;
		if (atomic_compare_exchange_strong_explicit(&tail, &prev, mcs_null, memory_order_release, memory_order_relaxed)) {
			return;  // if successor is really last one of MCS_node, just unlock
		}
		while (mcs_null == (successor = atomic_load_explicit(&node->next, memory_order_acquire))) {
			sleep(0);
			// asm("pause");
			// wait until successor is really empty
		}
	}

	atomic_store_explicit(&successor->lock, false, memory_order_release);
}
void thread() {
	mcs_node* node = malloc(sizeof(mcs_node));
	int cpu;
	struct timespec start;
	struct timespec current;
	clock_gettime(CLOCK_REALTIME, &start);
	clock_gettime(CLOCK_REALTIME, &current);
	int diff = current.tv_sec - start.tv_sec;
	while (diff < 100) {
		spin_lock(node);  // lock
		// CS
		cpu = sched_getcpu();
		counter[diff][cpu]++;
		for (int i = 0; i < 300; i++) {
			globalData[i] += i;
		}
		// CS
		spin_unlock(node);  // unlock
		clock_gettime(CLOCK_REALTIME, &current);
		diff = current.tv_sec - start.tv_sec;
	}
	free(node);
	return;
}

int main() {
	// init
	num_of_thread = num_of_vcore * num_of_vcore;
	atomic_init(&tail, mcs_null);
	// create and join thread
	pthread_t* tid = (pthread_t*)malloc(sizeof(pthread_t) * num_of_thread);
	for (int i = 0; i < num_of_thread; i++) {
		pthread_create(&tid[i], NULL, (void*)thread, NULL);
	}

	for (int i = 0; i < num_of_thread; i++)
		pthread_join(tid[i], NULL);

	free(tail);
	printf("lock is all done\n");
	// lock program is done, print out result

	FILE* out = fopen("MCS_cost", "w");
	if (!out) {
		printf("fail to open file\n");
		return 0;
	}
	for (int i = 0; i < 100; i++) {
		fprintf(out, "#%d ", i);
		for (int k = 0; k < num_of_vcore; k++) {
			fprintf(out, "%d ", counter[i][k]);
		}
		fprintf(out, "\n");
	}
	fclose(out);
	return 0;
}
