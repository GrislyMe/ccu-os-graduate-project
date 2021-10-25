#include "lock.h"

int rs_set_size = 11;
int rs_set[] = {160000, 120000, 80000, 40000, 20000, 10000, 5000, 2500, 1000, 500, 100};

struct timespec time_diff(struct timespec start, struct timespec end) {
	struct timespec tmp;
	if (end.tv_nsec - start.tv_nsec < 0) {
		tmp.tv_sec = end.tv_sec - start.tv_sec - 1;
		tmp.tv_nsec = end.tv_nsec - start.tv_nsec + 1000000000;
	} else {
		tmp.tv_sec = end.tv_sec - start.tv_sec;
		tmp.tv_nsec = end.tv_nsec - start.tv_nsec;
	}
	return tmp;
}
