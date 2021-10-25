#include "lock.h"

atomic_int lock = 0;

void spin_init() {
	atomic_store(&lock, 0);
}

void spin_lock() {
	int zero = 0;
	while (atomic_compare_exchange_weak(&lock, &zero, 1)) {
		zero = 0;
		asm("pause");
	}
}

void spin_unlock() {
	atomic_store(&lock, 0);
}
