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
long RS_size = 5000;
pthread_spinlock_t lock;
struct timespec previous;
struct timespec main_clock;
double global_data[16];

void thread()
{   
    struct timespec current;
    struct timespec end_time;
    long time_diff_nsec;
    long time_diff_sec;

    while(1){
        // spin lock
        pthread_spin_lock(&lock);
        
        // critical section
        clock_gettime(CLOCK_REALTIME, &current);

        for(int i=0; i<16; i++){
            global_data[i] *= 2.5;
        }

        time_diff_sec = (current.tv_sec - previous.tv_sec);
        time_diff_nsec = (current.tv_nsec - previous.tv_nsec);
        int cpu = sched_getcpu();
        printf("{ %ld sec %ld nsec } , { %d -> %d }\n", time_diff_sec, time_diff_nsec, pre_cpu, cpu);
        pre_cpu = cpu;
        
        pthread_spin_unlock(&lock);
        clock_gettime(CLOCK_REALTIME, &end_time);

        //RS
        while(1){
            clock_gettime(CLOCK_REALTIME, &current);
            if((current.tv_nsec - end_time.tv_nsec) > RS_size){
                previous = current;
                break;
            }
        }
    }
}

int main()
{
    // spin lock init
    pthread_spin_init(&lock, PTHREAD_PROCESS_PRIVATE);
    
    // set the number of threads
    int num_of_thread = 64;
    
    // main function info
    int cpu = sched_getcpu();
    clock_gettime(CLOCK_REALTIME, &main_clock);
    previous = main_clock;

    // create and join thread
    pthread_t* tid = (pthread_t*)malloc(sizeof(pthread_t) * num_of_thread);

    for(int i=0; i<num_of_thread; i++){
        pthread_create(&tid[i], NULL, (void*)thread, (void*)i);
    }

    for(int i=0; i<num_of_thread; i++){
        pthread_join(tid[i], NULL);
    }
    
    pthread_spin_destroy(&lock);
    return 0;
}
