#define _GNU_SOURCE
#include "../lib/lock.h"
#include <pthread.h>
#include <stdatomic.h>
#include <stdio.h>
#include <stdlib.h>

#define num_of_vcore 6
#define RS_size 100

long long int counter;
int globalData[300];

void thread(long rs) {
	struct timespec start;
    struct timespec current;
    struct timespec rs_start;
    struct timespec rs_end;
    clock_gettime(CLOCK_REALTIME, &start);
    clock_gettime(CLOCK_REALTIME, &current);
    int diff = current.tv_sec - start.tv_sec;
	
	while (diff < 10) {
		ticket_lock();  // lock
		// CS
        for (int i = 0; i < 300; i++) {
			globalData[i] += i;
		}
        counter++;
		// CS
		ticket_unlock();  // unlock
		clock_gettime(CLOCK_REALTIME, &current);
        while(clock_gettime(CLOCK_REALTIME, &rs_end) && (rs_end.tv_nsec - rs_start.tv_nsec) < rs)
            ;
        clock_gettime(CLOCK_REALTIME, &current);
        diff = current.tv_sec - start.tv_sec;
	}
}

int main() {
	// init
	int num_of_thread = num_of_vcore;  // * num_of_vcore;
    int rs_set[] = {160000, 120000, 80000, 40000, 20000, 10000, 5000, 1000, 500, 100};

	for(int j = 0; j < 10; j++){
        counter = 0;
        printf("%d\n", rs_set[j]);
        pthread_t* tid = (pthread_t*)malloc(sizeof(pthread_t) * num_of_thread);

    	for (int i = 0; i < num_of_thread; i++) {
	    	pthread_create(&tid[i], NULL, (void*)thread, (void*)rs_set[j]);
    	}

    	for (int i = 0; i < num_of_thread; i++)
	    	pthread_join(tid[i], NULL);

        FILE* out = fopen("ticket_lps", "a");
        if(!out){
            printf("fail to open file\n");
            return 0;
        }
        fprintf(out, "%d %lld\n", rs_set[j], counter);
        fclose(out);
    }

	
	return 0;
}
