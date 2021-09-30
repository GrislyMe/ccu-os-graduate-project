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
mcs_node* head = NULL;
_Atomic int progress = 0;

bool atomic_append(mcs_node* node) {
	if (atomic_load(&head) == NULL) {
		atomic_store(&head, node);
		atomic_store(&tail, node);
		atomic_store(&node->lock, 0);
		// head = node;
		// tail = node;
		// node->lock = 0;
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
}

void spin_lock(int tid) {
	mcs_node* node = malloc(sizeof(mcs_node));
	node->lock = 1;
	node->next = NULL;
	node->tid = tid;
	atomic_thread_fence(memory_order_acquire);
	if (atomic_append(node)) {
		return;
	}

	while (node->lock != 0) {
		if (head == NULL) {
			printf("id:%d, head is missing in LS\n", node->tid);
			scanf("%d", &progress);
		}
		sleep(0);
	}
}

void spin_unlock() {
	mcs_node* successor;
	successor = head->next;
	if (successor == NULL) {
		if (head == tail) {
			free(head);
			head = NULL;
			return;
		}
		while (!(successor = head->next)) {
			printf("waiting head->next exist\n");
			sleep(0);
		}
	}
	printf("id:%d, got the lock\n", successor->tid);
	atomic_store(&successor->lock, 0);
}
void thread() {
	spin_lock(gettid());  // lock
	// CS
	if (progress != 0) {
		printf("//////////////////progress fail\n");
	}
	atomic_fetch_add(&progress, 1);
	atomic_fetch_add(&progress, -1);
	// usleep(100);
	// CS
	spin_unlock();  // unlock
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
