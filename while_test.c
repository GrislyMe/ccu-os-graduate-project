#define _GNU_SOURCE
#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include <pthread.h>
#include <stdatomic.h>
#include <signal.h>
#include <unistd.h>
#include <strings.h>
#include <sys/syscall.h>
#include <assert.h>
#include <string.h>

int pre_cpu = -1;
pthread_spinlock_t lock;
struct timespec previous;
double global_data[128];
long long int past_times[16][16], done_times[16][16];
int neg;
int num_cpu;
cpu_set_t set;
int** thread_data;

void thread(void* assign_cpu)
{
    struct timespec current;
	cpu_set_t setthread;
	CPU_ZERO(&setthread);
	CPU_SET((int)assign_cpu, &setthread);
	pthread_setaffinity_np(pthread_self(), sizeof(cpu_set_t), &setthread);
	int cpu = sched_getcpu();
	long diff_nsec;
	static _Thread_local int var = 25;
	thread_data[(int)assign_cpu] = &var;
	
	for(int k=1; k<=10000000 ;k++){
        // spin lock
        pthread_spin_lock(&lock);
		if(pre_cpu == -1){
			pre_cpu = cpu;
			pthread_spin_unlock(&lock);
			continue;
		}
        clock_gettime(CLOCK_REALTIME, &previous);
        // critical section
		// reading thread local data from pre_cpu
		for(int i=0;i<50;i++){
			(*thread_data[ pre_cpu ]) *= 7;
			var += (*thread_data[ pre_cpu ]) + 11;
		}
        clock_gettime(CLOCK_REALTIME, &current);
		//// gettime after changing global_data
		diff_nsec = current.tv_nsec - previous.tv_nsec;
		
		if(diff_nsec < 5000 && diff_nsec > 0){
			//printf("%ld nsec, %d -> %d\n", diff_nsec, pre_cpu, cpu);
			past_times[ pre_cpu ][ cpu ] += diff_nsec;
			done_times[ pre_cpu ][ cpu ] ++;
		}else{
			//printf("Error: %d to %d is %ld\n",pre_cpu, cpu, diff_nsec);
		}
		//// log diffrent of current process and previous process
		//// prevent MISS, take of which over 3000
		pre_cpu = cpu;
        //previous = current;
		//// write finish time let next thread can compute pasted time

        // unlock
		pthread_spin_unlock(&lock);
    }
}

int main()
{
    // spin lock init
    pthread_spin_init(&lock, PTHREAD_PROCESS_PRIVATE);

	// set the number of threads
    num_cpu = get_nprocs();
	int num_of_thread = num_cpu;
	
	// main function info
    previous.tv_nsec = 0;
	thread_data = malloc(sizeof(int*) * num_of_thread);

	// open file to write
	FILE *out = fopen("result", "w");
	// global matrix init
	for(int i=0; i<num_cpu; i++){
		for(int j=0; j<num_cpu; j++){
			past_times[i][j] = 0;
			done_times[i][j] = 0;
		}
	}
    // create and join thread
    pthread_t* tid = (pthread_t*)malloc(sizeof(pthread_t) * num_of_thread);
	for(int i=0; i<num_of_thread; i++){
		pthread_create(&tid[i], NULL, (void*)thread, (void*)i);
    }
    for(int i=0; i<num_of_thread; i++){
        pthread_join(tid[i], NULL);
    }
	// write matrix to result FILE
    for(int i=0; i<num_cpu; i++){
		for(int j=0; j<num_cpu; j++){
			if(done_times[i][j] > 0)
			fprintf(out,"{ 0 sec %lld nsec } , { %d -> %d }\n", past_times[i][j]/done_times[i][j], i, j);
		}
	}
	// free and close if need
    pthread_spin_destroy(&lock);
	fclose(out);
	//printf("neg = %d\n",neg);
    return 0;
}
