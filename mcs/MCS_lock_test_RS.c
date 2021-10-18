#define _GNU_SOURCE
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include "../lib/lock.h"
// normal define
#define num_of_vcore 6

int RS_size, nowRS;
int RS_list[10] = {160000, 120000, 80000, 40000, 20000, 10000, 5000, 1000, 500, 100};
int num_of_thread;
long long int counter[10] = {0};
int globalData[300];
struct timespec thread_time[64];
int test;
int lt, ut;

void thread() {
	mcs_node* node = malloc(sizeof(mcs_node));
	int cpu;
	struct timespec start;
	struct timespec current;
	struct timespec t;
	clock_gettime(CLOCK_REALTIME, &start);
	clock_gettime(CLOCK_REALTIME, &current);
	int diff = current.tv_sec - start.tv_sec;
	while (diff < 10) {
		spin_lock(node);  // lock	
		cpu = sched_getcpu();
		// CS
		counter[nowRS]++;
		//printf("++ [%d]=%lld\n", nowRS, counter[nowRS]);
		for (int i = 0; i < 300; i++) {
			/*if(i > 100)
				globalData[i] += i / 127 + i * 2 - 11;
			if(i > 200)
				globalData[i] += i*i / 1337;*/
			globalData[i] += i;
		}
		// CS
		spin_unlock(node);  // unlock
		clock_gettime(CLOCK_REALTIME, &t);
		while(clock_gettime(CLOCK_REALTIME, &current) && (current.tv_nsec - t.tv_nsec ) < RS_size){
			//doing nothing
		}
		clock_gettime(CLOCK_REALTIME, &current);
		diff = current.tv_sec - start.tv_sec;
	}
	free(node);
	return;
}

int main() {
	// init
	num_of_thread = num_of_vcore;
	tail = malloc(sizeof(mcs_node));
	lt=0, ut=0;
	for(int T=0;T<10;T++){
		nowRS = T;
		RS_size = RS_list[nowRS];
		atomic_init(&tail, mcs_null);
		// create and join thread
		pthread_t* tid = (pthread_t*)malloc(sizeof(pthread_t) * num_of_thread);
		for (int i = 0; i < num_of_thread; i++) {
			pthread_create(&tid[i], NULL, (void*)thread, NULL);
		}
	
		for (int i = 0; i < num_of_thread; i++)
			pthread_join(tid[i], NULL);
	}
	free(tail);
	// printf("lock is all done\n");
	// lock program is done, print out result
	FILE* out = fopen("MCS_RS_test", "w");
	if (!out) {
		printf("fail to open file\n");
		return 0;
	}
	for(int T=0;T<10;T++){
		fprintf(out, "%d %lld\n", RS_list[T], counter[T]);
	}
	fclose(out);
	return 0;
}
