#define _GNU_SOURCE
#include "../lib/lock.h"
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/sysinfo.h>
#include <unistd.h>

#define error_mask 4000
// error_mask will limit the ns of a thread switch
// for how to determine the value can observe the core(n) to core(n) time
// cause we already use sleep(0)
// any core(n) to core(n) will count as an error

pthread_spinlock_t plock;
long long int timeCost[16][16] = {0};
long long int counter[16][16] = {0};
int pre_cpu;
double* globalData;
struct timespec previous;

void thread(int arg) {
	struct timespec current;
	int cpu;
	long diff;

	for (int t = 0; t < 100; t++) {
		// spin lock
		pthread_spin_lock(&plock);
		// critical section

		// load data from last modifyed core or memory
		for (int i = 0; i < 128; i++)
			globalData[i] *= 25.242;

		cpu = sched_getcpu();
		clock_gettime(CLOCK_MONOTONIC, &current);

		// remove some false data
		diff = time_diff(previous, current).tv_nsec;
		if (diff < error_mask) {
			timeCost[pre_cpu][cpu] += diff;
			counter[pre_cpu][cpu]++;
		}

		// reset timer
		clock_gettime(CLOCK_MONOTONIC, &current);
		pre_cpu = cpu;
		previous = current;

		// spin unlock
		pthread_spin_unlock(&plock);

		// if (arg)
		// pthread_yield();
		sleep(0);
	}
}

int main() {
	// init
	const int num_of_vcore = get_nprocs();
	const int num_of_thread = num_of_vcore;

	globalData = (double*)malloc(sizeof(double) * 128);
	pthread_spin_init(&plock, PTHREAD_PROCESS_PRIVATE);
	int* arg = (int*)malloc(sizeof(int));

	// int scheduler = SCHED_FIFO;
	// struct sched_param param = {.sched_priority = 0};
	// param.sched_priority = sched_get_priority_max(scheduler);
	// sched_setscheduler(0, scheduler, &param);
	// printf("scheduler: %d scheduler priority: %d\n", sched_getscheduler(0), param.sched_priority);

	// clock and cpu init
	pre_cpu = sched_getcpu();
	clock_gettime(CLOCK_REALTIME, &previous);

	cpu_set_t cpusets[num_of_vcore];
	for (int i = 0; i < num_of_vcore; i++) {
		CPU_ZERO(&cpusets[i]);
		CPU_SET(i, &cpusets[i]);
	}

	// create and join thread
	pthread_t* tid = (pthread_t*)malloc(sizeof(pthread_t) * num_of_thread);

	// pthread_spin_lock(&plock);
	for (int i = 0; i < num_of_thread; i++) {
		*arg = i < (num_of_thread - num_of_vcore);
		pthread_create(&tid[i], NULL, (void*)thread, arg);
		pthread_setaffinity_np(tid[i], sizeof(cpu_set_t), &cpusets[i % num_of_vcore]);
	}
	pthread_spin_unlock(&plock);

	for (int i = 0; i < num_of_thread; i++)
		pthread_join(tid[i], NULL);

	pthread_spin_destroy(&plock);

	FILE* out = fopen("result", "a+");
	for (int i = 0; i < num_of_vcore; i++)
		for (int k = 0; k < num_of_vcore; k++)
			if (counter[i][k])
				fprintf(out, "%lld %lld %d %d\n", timeCost[i][k], counter[i][k], i, k);

	fclose(out);
	return 0;
}
