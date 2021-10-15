#define _GNU_SOURCE
#include "../lib/lock.h"
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
// normal define
#define num_of_vcore 6
int RS_size, nowRS;
int RS_list[10];
int num_of_thread;
long long int counter[10][num_of_vcore] = {0};
int globalData[300];
struct timespec thread_time[64];

// mcs define
void thread() {
	mcs_node* node = malloc(sizeof(mcs_node));
	int cpu;
	struct timespec start;
	struct timespec current;
	struct timespec t;
	clock_gettime(CLOCK_REALTIME, &start);
	clock_gettime(CLOCK_REALTIME, &current);
	int diff = current.tv_sec - start.tv_sec;
	while (diff < 100) {
		spin_lock(node);  // lock
		
		// CS
		cpu = sched_getcpu();
		counter[nowRS][cpu]++;
		for (int i = 0; i < 300; i++) {
			globalData[i] += i;
		}
		// CS
		spin_unlock(node);  // unlock
		clock_gettime(CLOCK_REALTIME, &t);
		while(1){
			clock_gettime(CLOCK_REALTIME, &current);
			if(current - t > RS_size){
				break;
			}
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
	RS_list = {160000, 120000, 80000, 40000, 20000, 10000, 5000, 1000, 500, 100};
	for(int T=0;T<10;T++){
		nowRS = T;
		RS_size = RS_list[nowRS];
		// create and join thread
		pthread_t* tid = (pthread_t*)malloc(sizeof(pthread_t) * num_of_thread);
		for (int i = 0; i < num_of_thread; i++) {
			pthread_create(&tid[i], NULL, (void*)thread, NULL);
		}
	
		for (int i = 0; i < num_of_thread; i++)
			pthread_join(tid[i], NULL);
		free(tail);
	}
	// printf("lock is all done\n");
	// lock program is done, print out result
	long long total = 0;
	FILE* out = fopen("MCS_RS_test", "w");
	if (!out) {
		printf("fail to open file\n");
		return 0;
	}
	for(int T=0;T<10;T++){
		fprintf(out, "%d ", RS_list[T]);
		total = 0;
		for (int k = 0; k < num_of_vcore; k++) {
			total += counter[T][k];
			//fprintf(out, "%d ", counter[k]);
		}
		fprintf(out, "%d\n", total);
	}
	fclose(out);
	return 0;
}
