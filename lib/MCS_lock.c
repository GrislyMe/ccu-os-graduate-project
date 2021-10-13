#define _GNU_SOURCE
#include "lock.h"
#include <stdbool.h>

_Atomic struct mcs_node* tail;
void spin_lock(mcs_node* node) {
	atomic_init(&node->next, mcs_null);
	mcs_node* prev = atomic_exchange_explicit(&tail, node, memory_order_acq_rel);
	////assign prev as tail, if prev exist, go waiting.
	//
	if (prev != mcs_null) {
		atomic_init(&node->lock, true);
		atomic_store_explicit(&prev->next, node, memory_order_release);

		////waiting
		while (atomic_load_explicit(&node->lock, memory_order_acquire)) {
			asm("pause");
		}
	}
}

void spin_unlock(mcs_node* node) {
	mcs_node* successor = atomic_load_explicit(&node->next, memory_order_acquire);
	// check successor
	if (successor == mcs_null) {
		mcs_node* prev = node;
		if (atomic_compare_exchange_strong_explicit(&tail, &prev, mcs_null, memory_order_release, memory_order_relaxed)) {
			return;  // if successor is really last one of MCS_node, just unlock
		}
		while (mcs_null == (successor = atomic_load_explicit(&node->next, memory_order_acquire))) {
			asm("pause");
			// wait until successor is really empty
		}
	}

	atomic_store_explicit(&successor->lock, false, memory_order_release);
}
