#include "storage_sync.h"

#include <assert.h>

void cmnStorageSyncPublishEvent(CmnStorageSync* sync) {
	cmnAtomicAdd(&sync->seq.value, 1U, CMN_RELEASE);
	cmnFutexBroadcast(&sync->seq);
}

void cmnStorageSyncWaitUntilNormal(CmnStorageSync* sync) {
	for (;;) {
		uint32_t seq = cmnAtomicLoad(&sync->seq.value, CMN_ACQUIRE);

		CmnStorageSyncState state = (CmnStorageSyncState)cmnAtomicLoad(&sync->state.value, CMN_ACQUIRE);
		if (state == CMN_STORAGE_SYNC_NORMAL) {
			return;
		}

		cmnFutexWait(&sync->seq, seq);
	}
}

void cmnStorageSyncMarkAsUsingResources(CmnStorageSync* sync) {
	cmnAtomicAdd(&sync->users.value, 1U, CMN_ACQ_REL);
}

void cmnStorageSyncMarkAsNotUsingResources(CmnStorageSync* sync) {
	uint32_t prev;
	uint32_t next;
	for (;;) {
		prev = cmnAtomicLoad(&sync->users.value, CMN_ACQUIRE);
		if (prev == 0) {
			assert(false && "cmnStorageSyncMarkAsNotUsingResources called with users == 0.");
			return;
		}

		next = prev - 1;
		if (cmnAtomicCompareExchangeStrong(&sync->users.value, &prev, next, CMN_ACQ_REL, CMN_ACQUIRE)) {
			break;
		}
	}

	if (next == 0) {
		cmnStorageSyncPublishEvent(sync);
	}
}

void cmnStorageSyncDeletionLock(CmnStorageSync* sync) {
	for (;;) {
		bool ok = cmnAtomicCompareExchangeStrong(
			&sync->state.value,
			(uint32_t)CMN_STORAGE_SYNC_NORMAL,
			(uint32_t)CMN_STORAGE_SYNC_PENDING_REMOVAL,
			CMN_ACQ_REL,
			CMN_ACQUIRE
		);
		if (ok) {
			cmnStorageSyncPublishEvent(sync);
			break;
		}

		uint32_t seq = cmnAtomicLoad(&sync->seq.value, CMN_ACQUIRE);
		cmnFutexWait(&sync->seq, seq);
	}

	cmnRWMutexLockWrite(&sync->mutex);

	for (;;) {
		uint32_t seq = cmnAtomicLoad(&sync->seq.value, CMN_ACQUIRE);

		uint32_t count = cmnAtomicLoad(&sync->users.value, CMN_ACQUIRE);
		if (count == 0) {
			break;
		}

		cmnFutexWait(&sync->seq, seq);
	}

	cmnAtomicStore(&sync->state.value, (uint32_t)CMN_STORAGE_SYNC_REMOVING, CMN_RELEASE);
	cmnStorageSyncPublishEvent(sync);
}

void cmnStorageSyncDeletionUnlock(CmnStorageSync* sync) {
	cmnAtomicStore(&sync->state.value, (uint32_t)CMN_STORAGE_SYNC_NORMAL, CMN_RELEASE);
	cmnStorageSyncPublishEvent(sync);

	cmnRWMutexUnlockWrite(&sync->mutex);
}

void cmnStorageSyncReleaseResource(CmnStorageSync* sync) {
	cmnStorageSyncMarkAsNotUsingResources(sync);
}

void cmnStorageSyncLockWrite(CmnStorageSync* sync) {
	cmnStorageSyncWaitUntilNormal(sync);
	cmnRWMutexLockWrite(&sync->mutex);
}

void cmnStorageSyncUnlockWrite(CmnStorageSync* sync) {
	cmnRWMutexUnlockWrite(&sync->mutex);
}

void cmnStorageSyncLockRead(CmnStorageSync* sync) {
	cmnStorageSyncWaitUntilNormal(sync);
	cmnRWMutexLockRead(&sync->mutex);
}

void cmnStorageSyncUnlockRead(CmnStorageSync* sync) {
	cmnRWMutexUnlockRead(&sync->mutex);
}

