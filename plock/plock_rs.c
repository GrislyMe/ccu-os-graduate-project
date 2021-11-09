#define _GNU_SOURCE
#include "../lib/lock.h"
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <threads.h>

#define num_of_vcore 16

// atomic_llong counter = 0;
int globalData[300];

void* thread(info* arg) {
	// set all the information that will be used later
	long long int counter = 0;
	struct timespec start;
	struct timespec current;
	struct timespec rs_start;
	clock_gettime(CLOCK_MONOTONIC, &start);
	clock_gettime(CLOCK_MONOTONIC, &current);

	while (1) {
		if (time_diff(start, current).tv_sec > 10)
			break;

		spin_lock();  // lock
		// CS
		for (int i = 0; i < 300; i++) {
			globalData[i] += i;
		}
		// atomic_fetch_add(&counter, 1);
		counter += 1;
		// CS
		spin_unlock();  // unlock
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
	int num_of_thread = num_of_vcore;
	info args[num_of_vcore];
	spin_init();
	for (int k = 0; k < rs_set_size; k++) {
		unsigned long long int ans = 0;
		printf("%d\n", rs_set[k]);
		pthread_t* tid = (pthread_t*)malloc(sizeof(pthread_t) * num_of_thread);

		for (int i = 0; i < num_of_thread; i++) {
			args[i].rs_size = rs_set[k];
			pthread_create(&tid[i], NULL, (void*)thread, &args[i]);
		}

		for (int i = 0; i < num_of_thread; i++) {
			pthread_join(tid[i], NULL);
			ans += args[i].lps;
		}

		FILE* out = fopen("plock_lps", "a");
		if (!out) {
			printf("fail to open file\n");
			return 0;
		}
		fprintf(out, "%d %lld\n", rs_set[k], ans);
		// counter = 0;
		fclose(out);
	}
	return 0;
}
