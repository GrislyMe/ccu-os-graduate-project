#define _GNU_SOURCE
#include <assert.h>
#include <errno.h>
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

int                pre_cpu = -1;
pthread_spinlock_t lock;
struct timespec    previous;
double             global_data[128];
long long int      past_times[16][16] = {0}, done_times[16][16] = {0};
int                neg;
int                num_cpu;
cpu_set_t          set;
int**              thread_data;

void thread(int assign_cpu) {
	struct timespec current;
	long            diff_nsec;
	// CPU assign (order by forloop in main function)
	cpu_set_t setthread;
	CPU_ZERO(&setthread);
	CPU_SET(assign_cpu, &setthread);
	pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &setthread);
	int cpu = sched_getcpu();
	// init _Thread_local variable in this thread,
	// and store its reference in global array
	static _Thread_local int var = 25;
	thread_data[assign_cpu]      = &var;

	// recommand k <= atleast 10^7
	for (int k = 1; k <= 500000; k++) {
		// spin lock
		pthread_spin_lock(&lock);
		// prevent main to thread make SEGFAULT
		if (pre_cpu == -1) {
			pre_cpu = cpu;
			pthread_spin_unlock(&lock);
			continue;
		}

		// get time before changing thread local data
		clock_gettime(CLOCK_REALTIME, &previous);
		// reading thread local data from pre_cpu
		for (int i = 0; i < 100; i++) {
			(*thread_data[pre_cpu]) = (*thread_data[pre_cpu]) * 2 + 7;
		}
		//// gettime after changing thread local data
		clock_gettime(CLOCK_REALTIME, &current);

		diff_nsec = current.tv_nsec - previous.tv_nsec;
		////if different of time is reasonable, write in global matrix
		if (diff_nsec < 5000 && diff_nsec > 0) {
			// printf("%ld nsec, %d -> %d\n", diff_nsec, pre_cpu, cpu);
			past_times[pre_cpu][cpu] += diff_nsec;
			done_times[pre_cpu][cpu]++;
		} else {
			// printf("Error: %d to %d is %ld\n",pre_cpu, cpu, diff_nsec);
		}
		// log cpuid as other thread's previous cpu
		pre_cpu = cpu;

		// unlock
		pthread_spin_unlock(&lock);
	}
}

int main() {
	// spin lock init
	pthread_spin_init(&lock, PTHREAD_PROCESS_PRIVATE);

	// set the number of threads
	num_cpu           = get_nprocs();
	int num_of_thread = num_cpu;

	// main function info
	previous.tv_nsec = 0;
	thread_data      = malloc(sizeof(int*) * num_of_thread);

	// open file to write
	FILE* out = fopen("result", "w");

	// create and join thread
	pthread_t* tid = (pthread_t*)malloc(sizeof(pthread_t) * num_of_thread);
	for (int i = 0; i < num_of_thread; i++) {
		pthread_create(&tid[i], NULL, (void*)thread, &i);
	}
	for (int i = 0; i < num_of_thread; i++) {
		pthread_join(tid[i], NULL);
	}
	// write matrix to result FILE
	for (int i = 0; i < num_cpu; i++) {
		for (int j = 0; j < num_cpu; j++) {
			if (done_times[i][j] > 0)
				fprintf(out, "{ 0 sec %lld nsec } , { %d -> %d }\n", past_times[i][j] / done_times[i][j], i, j);
		}
	}
	// free and close if need
	pthread_spin_destroy(&lock);
	fclose(out);
	// printf("neg = %d\n",neg);
	return 0;
}
