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
int num_of_vcore;
int num_of_thread;
// the structure of my mcs_lock_node
typedef struct mcs_node {
	_Atomic struct mcs_node* next;
	int tid;
	atomic_bool lock;
} mcs_node;

_Atomic struct mcs_node* tail;
#define mcs_null (struct mcs_node*)NULL
_Atomic int progress = 0;

void spin_lock(mcs_node* node) {
	atomic_init(&node->next, mcs_null);
	mcs_node* prev = atomic_exchange_explicit(&tail, node, memory_order_acq_rel);
	if (prev != mcs_null) {
		atomic_init(&node->lock, true);
		atomic_store_explicit(&prev->next, node, memory_order_release);

		////waiting
		int times = 0;
		while (atomic_load_explicit(&node->lock, memory_order_acquire)) {
			if (times++ > 1000000) {
				printf("%d lost in LS\n", node->tid);
			}
			sleep(0);
		}
	}
}

void spin_unlock(mcs_node* node) {
	mcs_node* successor = atomic_load_explicit(&node->next, memory_order_acquire);
	if (successor == mcs_null) {
		mcs_node* prev = node;
		if (atomic_compare_exchange_strong_explicit(&tail, &prev, mcs_null, memory_order_release, memory_order_relaxed)) {
			return;
		}
		int times = 0;
		while (mcs_null == (successor = atomic_load_explicit(&node->next, memory_order_acquire))) {
			if (times++ > 1000000) {
				printf("%d give up safety\n", node->tid);
				return;
			}
		}
	}

	atomic_store_explicit(&successor->lock, false, memory_order_release);
}
void thread() {
	mcs_node* node = malloc(sizeof(mcs_node));
	node->tid = gettid();
	spin_lock(node);  // lock
	// CS
	if (progress != 0) {
		printf("//////////////////progress fail\n");
	}
	atomic_fetch_add(&progress, 1);
	atomic_fetch_add(&progress, -1);
	usleep(100);
	// CS
	spin_unlock(node);  // unlock
	free(node);
}

int main() {
	// init
	num_of_vcore = get_nprocs();
	num_of_thread = num_of_vcore * num_of_vcore * num_of_vcore;
	atomic_init(&tail, mcs_null);
	// create and join thread
	pthread_t* tid = (pthread_t*)malloc(sizeof(pthread_t) * num_of_thread);

	for (int i = 0; i < num_of_thread; i++) {
		pthread_create(&tid[i], NULL, (void*)thread, NULL);
	}

	for (int i = 0; i < num_of_thread; i++)
		pthread_join(tid[i], NULL);
	printf("////////program done////////\n");
	// free(tail);
	return 0;
}
