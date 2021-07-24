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

#define num_of_thread 64
#define v_core get_nprocs()
#define cpu sched_getcpu()

pthread_spinlock_t lock;
long long int timeCost[16][16] = {0};
int counter[16][16] = {0};
volatile int pre_cpu;
volatile double* globalData;
struct timespec previous;

void thread() {
	struct timespec current;
	long diff;

	for (int t = 0; t < 100; t++) {
		// spin lock
		pthread_spin_lock(&lock);
		// critical section

		for (int i = 0; i < 128; i++)
			globalData[i] *= 25.242;

		clock_gettime(CLOCK_REALTIME, &current);
		diff = current.tv_nsec - previous.tv_nsec;
		if (diff > 0 && diff < 10000) {
			timeCost[pre_cpu][cpu] += diff;
			counter[pre_cpu][cpu]++;
		}

		pre_cpu = cpu;
		previous = current;

		// spin unlock
		pthread_spin_unlock(&lock);
	}
}

int main() {
	// init
	globalData = (double*)malloc(sizeof(double) * 128);
	pthread_spin_init(&lock, PTHREAD_PROCESS_PRIVATE);

	// create and join thread
	pthread_t* tid = (pthread_t*)malloc(sizeof(pthread_t) * num_of_thread);

	// clock and cpu init
	pre_cpu = cpu;
	clock_gettime(CLOCK_REALTIME, &previous);

	for (int i = 0; i < num_of_thread; i++)
		pthread_create(&tid[i], NULL, (void*)thread, NULL);

	for (int i = 0; i < num_of_thread; i++)
		pthread_join(tid[i], NULL);

	pthread_spin_destroy(&lock);

	FILE* out = fopen("result", "a+");
	for (int i = 0; i < v_core; i++)
		for (int k = 0; k < v_core; k++)
			if (counter[i][k])
				fprintf(out, "{ %lld nsec } , { %d -> %d }\n", timeCost[i][k] / counter[i][k], i, k);

	fclose(out);
	return 0;
}
