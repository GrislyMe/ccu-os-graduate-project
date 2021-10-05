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
	struct mcs_node* next;
	int tid;
	int lock;
} mcs_node;
mcs_node* tail = NULL;
// mcs_node* head = NULL;
mcs_node* empty = NULL;
_Atomic int progress = 0;

/*bool atomic_append(mcs_node* node) {
    if (atomic_load(&head) == NULL) {
        atomic_store(&head, node);
        atomic_store(&tail, node);
        atomic_store(&node->lock, 0);
        atomic_thread_fence(memory_order_release);
        printf("id:%d, take the head\n", node->tid);
        return true;
    } else {
        atomic_store_explicit(&tail->next, node, memory_order_seq_cst);
        atomic_store_explicit(&tail, node, memory_order_relaxed);
        // tail->next = node;
        // tail = node;
        printf("id:%d, append\n", node->tid);
        atomic_thread_fence(memory_order_release);
        return false;
    }
}*/

void spin_lock(mcs_node* node) {
	empty = NULL;
	if (atomic_compare_exchange_strong(&tail, &empty, node) == 1) {
		printf("%d just get the tail\n", node->tid);
		return;
	} else {
		node->lock = 1;
		atomic_thread_fence(memory_order_release);
		atomic_store_explicit(&tail->next, node, memory_order_relaxed);
		atomic_store_explicit(&tail, tail->next, memory_order_seq_cst);
		// empty = NULL;
		// atomic_store_explicit(&node->next, empty, memory_order_release);
		// waiting get lock
		int times = 0;
		while (node->lock != 0) {
			if (times++ > 9999999) {
				printf("%d lost in LS\n", node->tid);
				scanf("%d", &progress);
			}
			asm("pause");
		}
	}
}

void spin_unlock(mcs_node* node) {
	mcs_node* successor;
	successor = node->next;
	empty = NULL;
	if (successor == empty) {
		if (atomic_compare_exchange_strong(&tail, &empty, node) == 1) {
			printf("%d drop the lock\n", node->tid);
			return;
		}
	}
	empty = NULL;
	int test = 0;
	while (successor == empty) {
		if (test++ > 9999999) {
			printf("%d don't care safety, just gone.\n", node->tid);
			return;
		}
	}
	// printf("%d pass lock to %d\n", node->tid, successor->tid);
	successor->lock = 0;
}
void thread() {
	atomic_thread_fence(memory_order_acquire);
	mcs_node* node = malloc(sizeof(mcs_node));
	node->lock = 0;
	empty = NULL;
	node->next = empty;
	node->tid = gettid();
	atomic_thread_fence(memory_order_release);
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
	// create and join thread
	pthread_t* tid = (pthread_t*)malloc(sizeof(pthread_t) * num_of_thread);

	for (int i = 0; i < num_of_thread; i++) {
		pthread_create(&tid[i], NULL, (void*)thread, NULL);
	}

	for (int i = 0; i < num_of_thread; i++)
		pthread_join(tid[i], NULL);
	printf("////////program done////////\n");
	return 0;
}
