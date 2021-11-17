#include "./lock.h"
#define _GNU_SOURCE
#include <sched.h>
#include <stdio.h>
#include <stdlib.h>
#include <threads.h>
// 2 -> 1 -> 0 -> 4 -> 5 -> 3
// ---------->    <----------
//  same ccx        same ccx
int* _idCov;  // this array should be changed on different CPU
int core;
int vcore;
int zero = 0;
thread_local int routingID;

atomic_int lock = 0;
// atomic_int *waitArray;
struct wait* waitArray;

void init_routingID(int cpu) {
	// cause for each thread it has to run once
	// therefore the thread local can be set
	routingID = _idCov[cpu];
}

void soa_spin_init(int num_of_vcore, int* tsp_order) {
	// set vcore number
	vcore = num_of_vcore;
	// core = num_of_vcore / 2;
	// init waitArray and idCov
	// waitArray = malloc(sizeof(atomic_int) * vcore);
	waitArray = malloc(sizeof(struct wait) * vcore);
	_idCov = malloc(sizeof(int) * vcore);
	for (int i = 0; i < num_of_vcore; i++) {
		// waitArray[i] = malloc(sizeof(int) * vcore);
		waitArray[i].value = 0;
		_idCov[i] = tsp_order[i];
	}
}

void spin_lock() {
	// init routingID
	// waitArray[routingID] = 1;
	waitArray[routingID].value = 1;
	while (1) {
		zero = 0;
		// let the variable "zero" always contain the value '0'
		// cause compare_exchange will also modify zero when compare is failed
		if (waitArray[routingID].value == 0)
			return;
		if (atomic_compare_exchange_strong(&lock, &zero, 1)) {
			waitArray[routingID].value = 0;
			return;
		}
	}
}

void spin_unlock() {
	for (int i = 1; i < vcore - 1; i++) {
		if (waitArray[(i + routingID) % vcore].value == 1) {
			waitArray[(i + routingID) % vcore].value = 0;
			return;
		}
	}
	lock = 0;
}
