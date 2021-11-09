#define _GNU_SOURCE
#include "../lib/lock.h"
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>

#define num_of_vcore 16

int globalData[300];

void* thread(info* arg) {
	unsigned long long int counter = 0;
	struct timespec start;
	struct timespec current;
	struct timespec rs_start;
	clock_gettime(CLOCK_MONOTONIC, &start);
	clock_gettime(CLOCK_MONOTONIC, &current);

	while (1) {
		if (time_diff(start, current).tv_sec > 10)
			break;
		ticket_lock();  // lock
		// CS
		for (int i = 0; i < 300; i++) {
			globalData[i] += i;
		}
		counter += 1;
		// CS
		ticket_unlock();  // unlock
		clock_gettime(CLOCK_MONOTONIC, &rs_start);
		while (1) {
			clock_gettime(CLOCK_MONOTONIC, &current);
			if (time_diff(rs_start, current).tv_nsec > arg->rs_size)
				break;
		}
	}
	arg->lps = counter;
	return 0;
}

int main() {
	// init
	int num_of_thread = num_of_vcore;  // * num_of_vcore;
	info args[num_of_thread];
	long long int ans = 0;

	for (int j = 0; j < rs_set_size; j++) {
		printf("%d\n", rs_set[j]);
		pthread_t* tid = (pthread_t*)malloc(sizeof(pthread_t) * num_of_thread);

		for (int i = 0; i < num_of_thread; i++) {
			args[i].rs_size = rs_set[j];
			pthread_create(&tid[i], NULL, (void*)thread, &args[i]);
		}

		for (int i = 0; i < num_of_thread; i++) {
			pthread_join(tid[i], NULL);
			ans += args[i].lps;
		}

		FILE* out = fopen("ticket_lps", "a");
		if (!out) {
			printf("fail to open file\n");
			return 0;
		}
		fprintf(out, "%d %lld\n", rs_set[j], ans);
		fclose(out);
	}

	return 0;
}
