#define _GNU_SOURCE
#include <assert.h>
#include <math.h>
#include <pthread.h>
#include <signal.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <sys/syscall.h>
#include <sys/sysinfo.h>
#include <unistd.h>

pthread_spinlock_t lock;
struct timespec previous;
int v_core;
int* pre_cpu;
int num_of_thread;
int counter[16][16] = {0};
int has_thread[16] = {0};
long long int timeCost[16][16] = {0};
double* globalData;

void thread() {
	struct timespec current;
	int cpu;
	long diff;

	// pthread_spin_lock(&lock);
	// cpu = sched_getcpu();
	// if (has_thread[cpu]) {
	//	num_of_thread--;
	//	pthread_spin_unlock(&lock);
	//	pthread_exit(0);
	//} else {
	//	has_thread[cpu] = 1;
	//}
	// pthread_spin_unlock(&lock);

	// while (num_of_thread <= 16) {
	//}

	for (int t = 0; t < 100; t++) {
		// spin lock
		pthread_spin_lock(&lock);
		// critical section
		cpu = sched_getcpu();

		for (int i = 0; i < 128; i++)
			globalData[i] *= 25.242;

		clock_gettime(CLOCK_REALTIME, &current);
		diff = current.tv_nsec - previous.tv_nsec;
		if (diff >= 0 && diff < 7000) {
			timeCost[*pre_cpu][cpu] += diff;
			counter[*pre_cpu][cpu]++;
		}

		*pre_cpu = cpu;
		previous = current;

		// spin unlock
		pthread_spin_unlock(&lock);
	}
}

int main() {
	// core num init
	v_core = get_nprocs();

	pre_cpu = malloc(sizeof(int));

	// globalData init
	globalData = (double*)malloc(sizeof(double) * 128);

	// spin lock init
	pthread_spin_init(&lock, PTHREAD_PROCESS_PRIVATE);

	// set the number of threads
	num_of_thread = 64;

	// create and join thread
	pthread_t* tid = (pthread_t*)malloc(sizeof(pthread_t) * num_of_thread);

	// pre_cpu* = cpu;
	*pre_cpu = sched_getcpu();
	clock_gettime(CLOCK_REALTIME, &previous);

	for (int i = 0; i < num_of_thread; i++)
		pthread_create(&tid[i], NULL, (void*)thread, NULL);

	for (int i = 0; i < num_of_thread; i++)
		pthread_join(tid[i], NULL);

	pthread_spin_destroy(&lock);

	// for (int i = 0; i < 16; i++)
	// 	if (!has_thread[i])
	// 		return 0;

	FILE* out = fopen("result", "a+");
	for (int i = 0; i < v_core; i++)
		for (int k = 0; k < v_core; k++)
			if (counter[i][k])
				fprintf(out, "{ 0 sec %lld nsec } , { %d -> %d }\n", timeCost[i][k] / counter[i][k], i, k);

	fclose(out);
	return 0;
}
