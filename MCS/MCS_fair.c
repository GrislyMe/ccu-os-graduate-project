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

volatile atomic_int GlobalLock = 0;
// static int zero = 0;
int num_of_vcore;
int num_of_thread;
double hi;
// the structure of my mcs_lock_node
typedef struct mcs_lock_node {
	struct mcs_lock_node* next;
	int tid;
	int lock;
} mcs_lock_node;
mcs_lock_node* tail = NULL;
mcs_lock_node* head = NULL;
mcs_lock_node* empty = NULL;
int appendOrder[1000];
int nowpos;
_Atomic int progress = 0;

bool atomic_append(mcs_lock_node* MCS) {
	atomic_thread_fence(memory_order_acquire);
	if (atomic_load_explicit(&head, memory_order_relaxed) == NULL) {
		// if there is no head, MCS become head
		atomic_store_explicit(&MCS->lock, 0, memory_order_relaxed);
		atomic_store_explicit(&head, MCS, memory_order_relaxed);
		atomic_store_explicit(&tail, MCS, memory_order_relaxed);
		printf("NewHead...but why?\n");
		appendOrder[0] = MCS->tid;
		atomic_thread_fence(memory_order_release);
		return true;
	} else {
		// normal append
		atomic_store_explicit(&tail->next, MCS, memory_order_relaxed);
		atomic_store_explicit(&tail, MCS, memory_order_relaxed);
		appendOrder[nowpos] = MCS->tid;
		printf("id:%d, append success, order is %d\n", MCS->tid, nowpos++);
		atomic_thread_fence(memory_order_release);
		return false;
	}
	// atomic_thread_fence( memory_order_release);
}

void spin_lock(int tid) {
	atomic_thread_fence(memory_order_acquire);
	// generate a node to append
	mcs_lock_node* MCS = malloc(sizeof(mcs_lock_node));
	atomic_store(&MCS->next, NULL);
	atomic_store(&MCS->tid, tid);
	atomic_store(&MCS->lock, 1);
	atomic_thread_fence(memory_order_release);
	if (atomic_append(MCS))
		return;

	// if MCS become head, aka got the lock to CS
	int times = 0;
	while (atomic_load(&MCS->lock) != 0) {
		// MCS didn't get the lock, waiting to unlock

		if (atomic_load(&head) == NULL) {  // it should not be happend
			// if there is no head, MCS become head before while loop
			// if there is a head, MCS should unlock in unlock section
			printf("head is gone, id: %d, still in lock\n", MCS->tid);
			if (tail != NULL)
				printf("tail still exist, it is id:%d\n", tail->tid);
			for (int i = 0; i < 1000; i++) {
				if (appendOrder[i] == MCS->tid) {
					printf("YouAreHere->");
					printf("%d: %d\n", i, appendOrder[i]);
				} else if (appendOrder[i] == 0) {
					break;
				}
			}
			scanf("%d", &progress);
		}
		/*if(times >= 100000){//someone missing in the lock section
		    printf("id:%d, is missing in lock\n", MCS->tid);
		    scanf("%d",&progress);
		}*/
		sleep(0);
		// asm("pause");
		times++;
	}
}

void spin_unlock() {
	atomic_thread_fence(memory_order_acquire);
	mcs_lock_node* successor;
	atomic_store_explicit(&successor, head->next, memory_order_relaxed);
	// generally head is passed CS and come in US, so lock can just trade to next MCS node
	// head->next is NULL means no more MCS_node behind the head
	// but there is BUG exist
	if (atomic_load_explicit(&successor, memory_order_relaxed) != NULL) {
		// trading lock to next MCS_node
		atomic_store_explicit(&successor->lock, 0, memory_order_relaxed);
		// free(head);//let me try to not free head
		atomic_store_explicit(&head, successor, memory_order_relaxed);
		printf("Now id:%d got the lock\n", successor->tid);
	} else {
		// nobody need lock, set head free
		printf("No more head to do, tail is id:%d\n, bye head", tail->tid);
		free(head);
		atomic_store_explicit(&head, NULL, memory_order_relaxed);
		atomic_store_explicit(&tail, NULL, memory_order_relaxed);
	}
	atomic_thread_fence(memory_order_release);
}
void thread() {

	// printf("got a thread id: %d\n", gettid());
	spin_lock(gettid());  // lock
	                      // CS
	if (progress != 0) {
		printf("//////////////////progress fail\n");
	}
	atomic_fetch_add(&progress, 1);
	for (int i = 0; i < 100000; i++) {
		hi += (i * 0.011 + 0.0017) / 1779;
	}
	if (hi > 7777777) {
		hi = 1;
	}
	printf("Who using id:%d, hi=%lf\n", gettid(), hi);
	atomic_fetch_add(&progress, -1);
	usleep(100);
	// CS
	spin_unlock();  // unlock
}

int main() {
	// init
	num_of_vcore = get_nprocs();
	num_of_thread = num_of_vcore * num_of_vcore * num_of_vcore;
	hi = 1;
	nowpos = 1;
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
