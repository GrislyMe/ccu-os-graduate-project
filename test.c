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

int idCov[6] = {4,2,0,1,5,3}; // this array should be changed on different CPU
thread_local int routingID;
atomic_int waitArray[16];
atomic_int GlobalLock = 0;
int zero = 0;

pthread_spinlock_t lock;
long long int timeCost[16][16] = {0};
int counter[16][16] = {0};
int pre_cpu;
double* globalData;
struct timespec previous;

void spin_init(){
    routingID = idCov[sched_getcpu()];
    for(int i=0; i<16; i++){
        waitArray[i] = 0;
    }
}

void spin_lock(){
    waitArray[routingID] = 1;
    while(atomic_load(&waitArray[routingID]) == 1){
        if(atomic_load(&waitArray[routingID]) == 0){
            return;
        }
        if(atomic_load(&GlobalLock) == 0 && atomic_compare_exchange_strong(&GlobalLock, &zero, 1)){
            atomic_store(&waitArray[routingID], 0); 
            return;
        }
    }
}

void spin_unlock()
{
    // the digit 6 is my core number
    // it should be changed on different CPU
    for(int i=1; i<6; i++){
        if(&waitArray[(i + routingID) % 6] == 1){
            atomic_store(&waitArray[(i + routingID) % 6], 0);
            atomic_store(&waitArray[routingID], -1);
            return;
        }
    }
    GlobalLock = 0;
}

void thread(int arg) {
    routingID = idCov[sched_getcpu()];

    spin_lock();
    printf("routingID : %d\n", routingID);
    spin_unlock(); 
    sleep(0.1);
}

int main() {
	// init
	const int num_of_vcore = get_nprocs();
	const int num_of_thread = num_of_vcore * num_of_vcore;
    spin_init();

	globalData = (double*)malloc(sizeof(double) * 128);
	pthread_spin_init(&lock, PTHREAD_PROCESS_PRIVATE);
	int* arg = (int*)malloc(sizeof(int));	

	// create and join thread
	pthread_t* tid = (pthread_t*)malloc(sizeof(pthread_t) * num_of_thread);

	//pthread_spin_lock(&lock);
    for (int i = 0; i < num_of_thread; i++) {
		*arg = i < (num_of_thread - num_of_vcore);
		pthread_create(&tid[i], NULL, (void*)thread, arg);
	}
	//pthread_spin_unlock(&lock);

	for (int i = 0; i < num_of_thread; i++)
		pthread_join(tid[i], NULL);

	//pthread_spin_destroy(&lock);

    printf("global lock : %d\n", GlobalLock);
	return 0;
}
