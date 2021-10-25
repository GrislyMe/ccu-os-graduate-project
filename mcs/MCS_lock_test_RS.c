#define _GNU_SOURCE
#include "../lib/lock.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
// normal define
#define num_of_vcore 16

int num_of_thread;
atomic_llong counter;
int globalData[300];

void thread(int rs_size) {
	mcs_node* node = malloc(sizeof(mcs_node));
	struct timespec start;
	struct timespec current;
	struct timespec rs_start;
	clock_gettime(CLOCK_MONOTONIC, &start);
	clock_gettime(CLOCK_MONOTONIC, &current);

	while (1) {
		if (time_diff(start, current).tv_sec > 10)
			break;

		spin_lock(node);
		// lock
		// CS
		for (int i = 0; i < 300; i++) {
			globalData[i] += i;
		}
		atomic_fetch_add(&counter, 1);
		// CS
		spin_unlock(node);
		// unlock

		clock_gettime(CLOCK_MONOTONIC, &rs_start);
		while (1) {
			clock_gettime(CLOCK_MONOTONIC, &current);
			if (time_diff(rs_start, current).tv_nsec > rs_size)
				break;
		}
	}
	free(node);
	return;
}

int main() {
	// init
	num_of_thread = num_of_vcore;
	for (int t = 0; t < rs_set_size; t++) {
		counter = 0;
		spin_init();
		// create and join thread
		pthread_t* tid = (pthread_t*)malloc(sizeof(pthread_t) * num_of_thread);
		for (int i = 0; i < num_of_thread; i++) {
			pthread_create(&tid[i], NULL, (void*)thread, (void*)rs_set[t]);
		}

		for (int i = 0; i < num_of_thread; i++)
			pthread_join(tid[i], NULL);
		FILE* out = fopen("mcs_lps", "a");
		if (!out) {
			printf("fail to open file\n");
			return 0;
		}
		fprintf(out, "%d %lld\n", rs_set[t], counter);
		fclose(out);
	}
	return 0;
}
