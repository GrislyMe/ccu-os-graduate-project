#define _GNU_SOURCE
#include "lock.h"

atomic_ulong gtk = 0;
unsigned long srv = 0;

void ticket_lock() {
	unsigned long ltk = atomic_fetch_add(&gtk, 1);
	while (ltk != srv)
		asm("pause");
}

void ticket_unlock() {
	srv++;
}
