#ifndef CMN_RWMUTEX_H
#define CMN_RWMUTEX_H

#include <lib/common/condition.h>

// Reader-writer mutex allowing one writer or many readers.
// Writers temporarily block new readers once queued.
// When both sides are queued, wakeups alternate preference.
typedef struct CmnRWMutex {
	// Internal mutex protecting shared RW state.
	CmnMutex mutex;

	// Condition for waiting readers.
	CmnCondition readers;
	// Condition for waiting writers.
	CmnCondition writers;

	uint32_t activeReaders;
	uint32_t waitingReaders;
	uint32_t waitingWriters;

	bool writerActive;
	// Preferred side for next wakeup when both queues are non-empty.
	bool preferWriter;
} CmnRWMutex;

// Acquire a shared lock.
void cmnRWMutexLockRead(CmnRWMutex* mutex);

// Try to acquire a shared lock without blocking.
bool cmnRWMutexTryLockRead(CmnRWMutex* mutex);

// Release a shared lock.
void cmnRWMutexUnlockRead(CmnRWMutex* mutex);

// Acquire an exclusive lock.
void cmnRWMutexLockWrite(CmnRWMutex* mutex);

// Try to acquire an exclusive lock without blocking.
bool cmnRWMutexTryLockWrite(CmnRWMutex* mutex);

// Release an exclusive lock.
void cmnRWMutexUnlockWrite(CmnRWMutex* mutex);

// RAII guard for read locking.
typedef class CmnScopedReadRWMutex {
public:
	// Managed mutex.
	CmnRWMutex* mutex;

	// Acquire a read lock on construction.
	CmnScopedReadRWMutex(CmnRWMutex* mutex) {
		this->mutex = mutex;
		cmnRWMutexLockRead(this->mutex);
	}

	// Acquire a read lock on construction.
	CmnScopedReadRWMutex(const CmnRWMutex* mutex) {
		this->mutex = (CmnRWMutex*)mutex;
		cmnRWMutexLockRead(this->mutex);
	}

	// Release the read lock on destruction.
	~CmnScopedReadRWMutex() {
		cmnRWMutexUnlockRead(this->mutex);
	}
} CmnScopedReadRWMutex;

// RAII guard for write locking.
typedef class CmnScopedWriteRWMutex {
public:
	// Managed mutex.
	CmnRWMutex* mutex;

	// Acquire a write lock on construction.
	CmnScopedWriteRWMutex(CmnRWMutex* mutex) {
		this->mutex = mutex;
		cmnRWMutexLockWrite(this->mutex);
	}

	// Acquire a write lock on construction.
	CmnScopedWriteRWMutex(const CmnRWMutex* mutex) {
		this->mutex = (CmnRWMutex*)mutex;
		cmnRWMutexLockWrite(this->mutex);
	}

	// Release the write lock on destruction.
	~CmnScopedWriteRWMutex() {
		cmnRWMutexUnlockWrite(this->mutex);
	}
} CmnScopedWriteRWMutex;

#endif // CMN_RWMUTEX_H
