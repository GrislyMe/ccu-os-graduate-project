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
// int idCov[num_of_vcore] = {3, 0, 4, 5, 7, 6, 2, 1, 11, 14, 13, 12, 9, 15, 10, 8};
// this array should be changed on different CPU
int rs_set[] = {160000, 120000, 80000, 40000, 20000, 10000, 5000, 1000, 500, 100};
long long int counter = 0;
// atomic_llong counter;
int globalData[300] = {0};

void thread(info* args) {
	int cid = args->cid;
	int rsID = args->rs_label;
	cpu_set_t cpuset;
	CPU_ZERO(&cpuset);
	CPU_SET(cid, &cpuset);
	sched_setaffinity(0, sizeof(cpuset), &cpuset);

	struct timespec start;
	struct timespec current;
	struct timespec rs_start;
	int diff;
	int rs = rs_set[rsID];

	clock_gettime(CLOCK_REALTIME, &start);
	clock_gettime(CLOCK_REALTIME, &current);
	diff = current.tv_sec - start.tv_sec;
	while (diff < 10) {
		spin_lock();  // lock
		// CS
		for (int i = 0; i < 300; i++)
			globalData[i] = globalData[i] + 1;
		counter++;
		// atomic_fetch_add(&counter, 1);
		// CS
		spin_unlock();  // unlock
		clock_gettime(CLOCK_REALTIME, &rs_start);
		while (clock_gettime(CLOCK_REALTIME, &current) && (current.tv_nsec - rs_start.tv_nsec) < rs)
			;
		clock_gettime(CLOCK_REALTIME, &current);
		diff = current.tv_sec - start.tv_sec;
	}
}

int main() {
	// init
	int num_of_thread = num_of_vcore;
	info args[num_of_vcore];
	for (int i = 0; i < num_of_vcore; i++) {
		args[i].cid = i;
	}

	for (int j = 0; j < 10; j++) {
		// create and join thread
		counter = 0;
		printf("%d\n", rs_set[j]);
		soa_spin_init(num_of_vcore, idCov);
		pthread_t* tid = (pthread_t*)malloc(sizeof(pthread_t) * num_of_thread);

		// pthread_spin_lock(&lock);
		for (int i = 0; i < num_of_thread; i++) {
			args[i].rs_label = j;
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
