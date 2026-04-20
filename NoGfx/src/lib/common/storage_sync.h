#ifndef CMN_STORAGESYNC_H
#define CMN_STORAGESYNC_H

#include <lib/common/futex.h>
#include <lib/common/rw_mutex.h>
#include <lib/common/handle_map.h>

// State machine used by CmnStorageSync.
typedef enum CmnStorageSyncState {
	CMN_STORAGE_SYNC_NORMAL = 0,
	CMN_STORAGE_SYNC_PENDING_REMOVAL,
	CMN_STORAGE_SYNC_REMOVING,
} CmnStorageSyncState;

// Synchronization primitive for handle-map-backed storages where resources can be
// removed asynchronously while other threads may still access them.
typedef struct CmnStorageSync {
	CmnRWMutex mutex;

	// Current CmnStorageSyncState value.
	CmnFutex state;
	// Number of active resource users outside rw lock.
	CmnFutex users;
	// Sequence futex used to wake waiters on relevant events.
	CmnFutex seq;
} CmnStorageSync;

// Publishes a synchronization event to wake storage waiters.
void cmnStorageSyncPublishEvent(CmnStorageSync* sync);

// Blocks until storage state returns to CMN_STORAGE_SYNC_NORMAL.
void cmnStorageSyncWaitUntilNormal(CmnStorageSync* sync);

// Marks beginning of resource use.
void cmnStorageSyncMarkAsUsingResources(CmnStorageSync* sync);

// Marks end of resource use.
void cmnStorageSyncMarkAsNotUsingResources(CmnStorageSync* sync);

// Acquires the deletion lock for the storage and waits until active users drain.
void cmnStorageSyncDeletionLock(CmnStorageSync* sync);

// Releases the deletion lock for the storage and unblocks new users.
void cmnStorageSyncDeletionUnlock(CmnStorageSync* sync);

void cmnStorageSyncLockWrite(CmnStorageSync* sync);

void cmnStorageSyncUnlockWrite(CmnStorageSync* sync);

void cmnStorageSyncLockRead(CmnStorageSync* sync);

void cmnStorageSyncUnlockRead(CmnStorageSync* sync);

// Acquires a resource referenced by handle and marks the storage as in-use when valid.
template <typename T>
T& cmnStorageSyncAcquireResource(CmnHandleMap<T>* map, CmnStorageSync* sync, CmnHandle handle, bool* wasHandleValid);

// Releases a previously acquired resource use marker.
void cmnStorageSyncReleaseResource(CmnStorageSync* sync);

// RAII guard wrapping cmnStorageSyncDeletionLock/cmnStorageSyncDeletionUnlock.
class CmnScopedStorageSyncDeletionLock {
public:
	CmnStorageSync* sync;

	CmnScopedStorageSyncDeletionLock(CmnStorageSync* sync) {
		this->sync = sync;
		cmnStorageSyncDeletionLock(this->sync);
	}

	~CmnScopedStorageSyncDeletionLock() {
		cmnStorageSyncDeletionUnlock(this->sync);
	}
};

class CmnScopedStorageSyncLockRead {
public:
	CmnStorageSync* sync;

	inline CmnScopedStorageSyncLockRead(CmnStorageSync* sync) {
		this->sync = sync;
		cmnStorageSyncLockRead(sync);
	}

	inline ~CmnScopedStorageSyncLockRead() {
		cmnStorageSyncUnlockRead(sync);
	}
};

class CmnScopedStorageSyncLockWrite {
public:
	CmnStorageSync* sync;

	inline CmnScopedStorageSyncLockWrite(CmnStorageSync* sync) {
		this->sync = sync;
		cmnStorageSyncLockWrite(sync);
	}

	inline ~CmnScopedStorageSyncLockWrite() {
		cmnStorageSyncUnlockWrite(sync);
	}
};

#include "storage_sync.inc"

#endif // CMN_STORAGESYNC_H
