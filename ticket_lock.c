#define _GNU_SOURCE
#include <pthread.h>
#include <stdatomic.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <threads.h>
#include <unistd.h>

#define num_of_vcore 6

atomic_ulong gtk = 0;
unsigned long srv = 0;
int counter[100][num_of_vcore] = {0};
int globalData[300];

void ticket_lock() {
	unsigned long ltk = atomic_fetch_add(&gtk, 1);
	while (ltk != srv)
		asm("pause");
}

void ticket_unlock() {
	srv++;
}

void thread() {
	int cpu;
	struct timespec start, current;
	clock_gettime(CLOCK_REALTIME, &start);
	clock_gettime(CLOCK_REALTIME, &current);
	int diff = current.tv_sec - start.tv_sec;

	while (diff < 100) {
		ticket_lock();  // lock

		// CS
		cpu = sched_getcpu();
		counter[diff][cpu]++;
		for (int i = 0; i < 300; i++) {
			globalData[i] += i;
		}
		// CS

		ticket_unlock();  // unlock

		clock_gettime(CLOCK_REALTIME, &current);
		diff = current.tv_sec - start.tv_sec;
	}
}

int main() {
	// init
	int num_of_thread = num_of_vcore;  // * num_of_vcore;

	// create and join thread
	pthread_t* tid = (pthread_t*)malloc(sizeof(pthread_t) * num_of_thread);

	// pthread_spin_lock(&lock);
	for (int i = 0; i < num_of_thread; i++) {
		pthread_create(&tid[i], NULL, (void*)thread, NULL);
	}
	// pthread_spin_unlock(&lock);

	for (int i = 0; i < num_of_thread; i++)
		pthread_join(tid[i], NULL);

	// pthread_spin_destroy(&lock);

	FILE* out = fopen("ticket_cost", "w");
	if (!out) {
		fprintf(stderr, "fail to open file\n");
		exit(0);
	}
	for (int i = 0; i < 100; i++) {
		fprintf(out, "#%d ", i);
		for (int k = 0; k < num_of_vcore; k++) {
			fprintf(out, "%d ", counter[i][k]);
		}
		fprintf(out, "\n");
	}
	fclose(out);
	return 0;
}
