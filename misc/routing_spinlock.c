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

#define num_of_vcore 16

// 2 -> 1 -> 0 -> 4 -> 5 -> 3
// ---------->    <----------
//  same ccx        same ccx
int idCov[num_of_vcore] = {3, 0, 4, 5, 7, 6, 2, 1, 15, 12, 14, 8, 11, 10, 9, 13};
int rs_set[] = {160000, 120000, 80000, 40000, 20000, 10000, 5000, 1000, 500, 100};
thread_local int routingID;
atomic_int GlobalLock = 0;
static int zero = 0;
long long int counter;
int globalData[300] = {0};

atomic_int waitArray[6];

typedef struct info {
	int rs_label;
	int cid;
} info;

void spin_init() {
	for (int i = 0; i < num_of_vcore; i++) {
		waitArray[i] = 0;
	}
}

void spin_lock(int routingID) {
	waitArray[routingID] = 1;
	while (1) {
		zero = 0;  // let the variable "zero" always contain the value '0'
		if (waitArray[routingID] == 0)
			return;
		if (atomic_compare_exchange_strong(&GlobalLock, &zero, 1)) {
			waitArray[routingID] = 0;
			return;
		}
	}
}

void spin_unlock(int routingID) {
	for (int i = 1; i < num_of_vcore - 1; i++) {
		if (waitArray[(i + routingID) % num_of_vcore] == 1) {
			waitArray[(i + routingID) % num_of_vcore] = 0;
			return;
		}
	}
	GlobalLock = 0;
}

void thread(void* args) {
	int cid = ((info*)args)->cid;
	int rsID = ((info*)args)->rs_label;
	routingID = idCov[cid];
	cpu_set_t cpuset;
	CPU_ZERO(&cpuset);
	CPU_SET(cid, &cpuset);
	sched_setaffinity(0, sizeof(cpuset), &cpuset);

	struct timespec start;
	struct timespec current;
	struct timespec rs_start;
	struct timespec rs_end;
	int diff;
	int rs = rs_set[rsID];

	clock_gettime(CLOCK_REALTIME, &start);
	clock_gettime(CLOCK_REALTIME, &current);
	diff = current.tv_sec - start.tv_sec;
	while (diff < 10) {
		spin_lock(routingID);  // lock
		// CS
		for (int i = 0; i < 48; i++)
			globalData[i] = globalData[i] + 1;
		counter++;
		// CS
		spin_unlock(routingID);  // unlock
		clock_gettime(CLOCK_REALTIME, &rs_start);
		while (clock_gettime(CLOCK_REALTIME, &rs_end) && (rs_end.tv_nsec - rs_start.tv_nsec) < rs)
			;
		clock_gettime(CLOCK_REALTIME, &current);
		diff = current.tv_sec - start.tv_sec;
	}
}

int main() {
	// init
	int num_of_thread = num_of_vcore;
	info args[num_of_vcore];
	for (int i = 0; i < 6; i++) {
		args[i].cid = i;
	}

	spin_init();

	for (int j = 0; j < 10; j++) {
		// create and join thread
		counter = 0;
		printf("%d\n", rs_set[j]);
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
