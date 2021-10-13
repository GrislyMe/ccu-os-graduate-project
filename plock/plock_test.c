#define _GNU_SOURCE
#include "../lib/lock.h"
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>

#define num_of_vcore 16

int counter[100][num_of_vcore] = {0};
int globalData[300];
struct timespec thread_time[64];

int thread() {
	// set all the information that will be used later
	int cpu;
	struct timespec start;
	struct timespec current;
	clock_gettime(CLOCK_REALTIME, &start);
	clock_gettime(CLOCK_REALTIME, &current);
	int diff = current.tv_sec - start.tv_sec;

	while (diff < 100) {
		spin_lock();  // lock
		// CS
		cpu = sched_getcpu();
		counter[diff][cpu]++;
		for (int i = 0; i < 300; i++) {
			globalData[i] += i;
		}
		// CS
		spin_unlock();  // unlock
		clock_gettime(CLOCK_REALTIME, &current);
		diff = current.tv_sec - start.tv_sec;
	}
	// clock_gettime(CLOCK_THREAD_CPUTIME_ID, &thread_time[*id]);
	return 0;
}

int main() {
	// init
	int num_of_thread = num_of_vcore;
	spin_init();

	pthread_t* tid = (pthread_t*)malloc(sizeof(pthread_t) * num_of_thread);

	for (int i = 0; i < num_of_thread; i++) {
		pthread_create(&tid[i], NULL, (void*)thread, NULL);
	}

	for (int i = 0; i < num_of_thread; i++)
		pthread_join(tid[i], NULL);

	FILE* out = fopen("plock_cost", "w");
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
