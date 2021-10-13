#define _GNU_SOURCE
#include "../lib/lock.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
// normal define
#define num_of_vcore 6

int num_of_thread;
int counter[100][num_of_vcore] = {0};
int globalData[300];
struct timespec thread_time[64];

// mcs define
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
	num_of_thread = num_of_vcore;
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
