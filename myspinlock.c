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
#include <threads.h>
#include <stdint.h>

int idCov[6] = {4,2,0,1,5,3};
thread_local int routingID;
atomic_int waitArray[16];
atomic_int GlobalLock = 0;
int zero = 0;

long long int timeCost[16][16] = {0};
int counter[16][16] = {0};
int pre_cpu;
double* globalData;

void spin_init(){
    routingID = idCov[sched_getcpu()];
    for(int i=0; i<16; i++){
        waitArray[i] = 0;
    }
}

void spin_lock(){
    waitArray[routingID] = 1;
    while(1){
        if(waitArray[routingID] == 0)
            return;
        if(atomic_compare_exchange_weak(&GlobalLock, &zero, 1)){
            waitArray[routingID] = 0;
            return;
        }
    }
}

void spin_unlock(){
    for(int i=1; i<5; i++){
        if(waitArray[(i + routingID) % 6] == 1){
            atomic_store(&waitArray[(i + routingID) % 6], 0);
            return;
        }
    }
    GlobalLock = 0;
}

void thread(int arg) {
    int cpu;

    for(int j=0; j<10; j++){
        cpu = sched_getcpu();    
        printf("%d using\n", cpu);

        spin_lock();
        // CS
        for(int i=0; i<128; i++){
            globalData[i] *= 2.3;
        }
        printf("No of cpu using in CS: %d\n", cpu);
        sleep(1);

        spin_unlock();

        //sleep(0);
    }
}

int main() {
	// init
	const int num_of_vcore = get_nprocs();
	const int num_of_thread = num_of_vcore * num_of_vcore;
    globalData = (double*)malloc(sizeof(double) * 128);
    spin_init();

	// create and join thread
	pthread_t* tid = (pthread_t*)malloc(sizeof(pthread_t) * num_of_thread);

	for (int i = 0; i < num_of_thread; i++) {
		pthread_create(&tid[i], NULL, (void*)thread, i);
	}

	for (int i = 0; i < num_of_thread; i++)
		pthread_join(tid[i], NULL);

	return 0;
}
