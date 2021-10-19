#define _GNU_SOURCE
#include "../lib/lock.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
// normal define
#define num_of_vcore 16

int rs_list[10] = {160000, 120000, 80000, 40000, 20000, 10000, 5000, 1000, 500, 100};
int num_of_thread;
atomic_llong counter;
int globalData[300];

void thread(int rs_size) {
	mcs_node* node = malloc(sizeof(mcs_node));
	struct timespec start;
	struct timespec current;
	struct timespec t;
	clock_gettime(CLOCK_REALTIME, &start);
	clock_gettime(CLOCK_REALTIME, &current);
	int diff = current.tv_sec - start.tv_sec;
	while (diff < 10) {
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
		clock_gettime(CLOCK_REALTIME, &t);
		while (clock_gettime(CLOCK_REALTIME, &current) && (current.tv_nsec - t.tv_nsec) < rs_size)
			;
		clock_gettime(CLOCK_REALTIME, &current);
		diff = current.tv_sec - start.tv_sec;
	}
	free(node);
	return;
}

int main() {
	// init
	num_of_thread = num_of_vcore;
	for (int t = 0; t < 10; t++) {
		counter = 0;
		spin_init();
		// create and join thread
		pthread_t* tid = (pthread_t*)malloc(sizeof(pthread_t) * num_of_thread);
		for (int i = 0; i < num_of_thread; i++) {
			pthread_create(&tid[i], NULL, (void*)thread, (void*)rs_list[t]);
		}

		for (int i = 0; i < num_of_thread; i++)
			pthread_join(tid[i], NULL);
		FILE* out = fopen("mcs_lps", "a");
		if (!out) {
			printf("fail to open file\n");
			return 0;
		}
		fprintf(out, "%d %lld\n", rs_list[t], counter);
		fclose(out);
	}
	return 0;
}
