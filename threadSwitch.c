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

pthread_spinlock_t lock;
struct timespec previous;
int pre_cpu = -1;

void thread() {
    struct timespec current;
    long time_diff_nsec;
    long time_diff_sec;

    // spin lock
    pthread_spin_lock(&lock);
    // critical section
    clock_gettime(CLOCK_REALTIME, &current);

    time_diff_sec = (current.tv_sec - previous.tv_sec);
    time_diff_nsec = (current.tv_nsec - previous.tv_nsec);
    printf("%ld sec %ld nsec: ", time_diff_sec, time_diff_nsec);
    int cpu = sched_getcpu();
    printf("%d -> %d\n", pre_cpu, cpu);
    sleep(0.5);

	pre_cpu = cpu;
    clock_gettime(CLOCK_REALTIME, &current);
    previous = current;

    // spin unlock
    pthread_spin_unlock(&lock);
}

int main() {
    // spin lock init
    pthread_spin_init(&lock, PTHREAD_PROCESS_PRIVATE);

    // set the number of threads
    int num_of_thread = 64;

    // main function info
    int cpu = sched_getcpu();
    struct timespec main_clock;
    //pre_cpu = cpu;
    //printf("Main function using CPU : %d\n", cpu);
    clock_gettime(CLOCK_REALTIME, &main_clock);
    previous = main_clock;

    // create and join thread
    pthread_t* tid = (pthread_t*)malloc(sizeof(pthread_t) * num_of_thread);

    for(int i=0; i<num_of_thread; i++)
        pthread_create(&tid[i], NULL, (void*)thread, (void*)i);

    for(int i=0; i<num_of_thread; i++)
        pthread_join(tid[i], NULL);

    pthread_spin_destroy(&lock);
    return 0;
}
