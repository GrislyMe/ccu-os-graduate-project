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
long long int timeCost[16][16] = {0};
int counter[16][16] = {0};
int pre_cpu;
double* globalData;
struct timespec previous;

void thread(void* arg) {
	struct timespec current;
	int cpu;
	long diff;

	for (int t = 0; t < 1000000; t++) {
		// spin lock
		pthread_spin_lock(&lock);
		// critical section

		for (int i = 0; i < 128; i++)
			globalData[i] *= 25.242;

		cpu = sched_getcpu();
		clock_gettime(CLOCK_REALTIME, &current);
		diff = current.tv_nsec - previous.tv_nsec;
		if (diff >= 0 && diff < 10000) {
			timeCost[pre_cpu][cpu] += diff;
			counter[pre_cpu][cpu]++;
		}

		clock_gettime(CLOCK_REALTIME, &current);
		pre_cpu = cpu;
		previous = current;

		// spin unlock
		pthread_spin_unlock(&lock);

		if (arg)
			pthread_yield();
	}
}

int main() {
	// init
	const int core_count = get_nprocs();
	const int num_of_thread = core_count * core_count;

	globalData = (double*)malloc(sizeof(double) * 128);
	pthread_spin_init(&lock, PTHREAD_PROCESS_PRIVATE);

	int scheduler = SCHED_FIFO;
	struct sched_param param = {.sched_priority = 0};
	param.sched_priority = sched_get_priority_max(scheduler);
	sched_setscheduler(0, scheduler, &param);
	printf("scheduler: %d scheduler priority: %d\n", sched_getscheduler(0), param.sched_priority);
	
	// clock and cpu init
	pre_cpu = sched_getcpu();
	clock_gettime(CLOCK_REALTIME, &previous);

	cpu_set_t cpusets[core_count];
	for (int i = 0; i < core_count; i++) {
		CPU_ZERO(&cpusets[i]);
		CPU_SET(i, &cpusets[i]);
	}

	// create and join thread
	pthread_t* tid = (pthread_t*)malloc(sizeof(pthread_t) * num_of_thread);
	
	pthread_spin_lock(&lock);
	for (int i = 0; i < num_of_thread; i++) {
		pthread_create(&tid[i], NULL, (void*)thread, i < (num_of_thread - core_count));
		pthread_setaffinity_np(tid[i], sizeof(cpu_set_t), &cpusets[i % core_count]);
	}
	pthread_spin_unlock(&lock);

	for (int i = 0; i < num_of_thread; i++)
		pthread_join(tid[i], NULL);

	pthread_spin_destroy(&lock);

	
	FILE* out = fopen("result", "a+");
	for (int i = 0; i < core_count; i++)
		for (int k = 0; k < core_count; k++)
			if (counter[i][k])
				fprintf(out, "%lld %d %d %d\n", timeCost[i][k], counter[i][k], i, k);

	fclose(out);
	return 0;
}
