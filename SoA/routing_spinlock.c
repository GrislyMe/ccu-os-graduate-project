#define _GNU_SOURCE
#include "../lib/lock.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <threads.h>

#define num_of_vcore 16

// 2 -> 1 -> 0 -> 4 -> 5 -> 3
// ---------->    <----------
//  same ccx        same ccx
int idCov[num_of_vcore] = {3, 0, 4, 5, 7, 6, 2, 1, 15, 12, 14, 8, 11, 10, 9, 13};
// this array should be changed on different CPU
// int idCov[6] = {4, 0, 1, 2, 5, 3};
atomic_llong counter = 0;
// atomic_llong counter;
int globalData[300] = {0};

void thread(info* args) {
	int cid = args->cid;
	int rs_size = args->rs_size;

	// assign each thread to use one vcore only
	cpu_set_t cpuset;
	CPU_ZERO(&cpuset);
	CPU_SET(cid, &cpuset);
	sched_setaffinity(0, sizeof(cpuset), &cpuset);

	struct timespec start;
	struct timespec current;
	struct timespec rs_start;
	// thread start time
	clock_gettime(CLOCK_MONOTONIC, &start);
	// init current time
	clock_gettime(CLOCK_MONOTONIC, &current);

	while (1) {
		// we count lock that was aquired during ten sec
		if (time_diff(start, current).tv_sec > 10)
			break;

		spin_lock();  // lock
		// CS
		for (int i = 0; i < 300; i++)
			globalData[i] = globalData[i] + 1;
		atomic_fetch_add(&counter, 1);
		// CS
		spin_unlock();  // unlock

		clock_gettime(CLOCK_MONOTONIC, &rs_start);
		// rs
		while (1) {
			clock_gettime(CLOCK_MONOTONIC, &current);
			if (time_diff(rs_start, current).tv_nsec > rs_size)
				break;
		}
	}
}

int main() {
	// init
	int num_of_thread = num_of_vcore;
	info args[num_of_vcore];
	// thread id init
	for (int i = 0; i < num_of_vcore; i++) {
		args[i].cid = i;
	}

	for (int j = 0; j < rs_set_size; j++) {
		// create and join thread
		counter = 0;
		printf("%d\n", rs_set[j]);
		soa_spin_init(num_of_vcore, idCov);
		pthread_t* tid = (pthread_t*)malloc(sizeof(pthread_t) * num_of_thread);

		// pthread_spin_lock(&lock);
		for (int i = 0; i < num_of_thread; i++) {
			// parse size and thread id
			args[i].rs_size = rs_set[j];
			pthread_create(&tid[i], NULL, (void*)thread, &args[i]);
		}

		// pthread_spin_unlock(&lock);
		for (int i = 0; i < num_of_thread; i++)
			pthread_join(tid[i], NULL);

		FILE* out = fopen("SoA_lps", "a");
		if (!out) {
			fprintf(stderr, "fail to open file\n");
			return 0;
		}
		fprintf(out, "%d %lld\n", rs_set[j], counter);
		fclose(out);
	}

	// pthread_spin_destroy(&lock);

	return 0;
}
