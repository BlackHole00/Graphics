#ifndef CMN_MUTEX_H
#define CMN_MUTEX_H

#include <lib/common/futex.h>
#include <lib/common/atomic.h>

// Internal state values of CmnMutex.
typedef enum CmnMutexState {
	// Mutex is not held by any thread.
	CMN_MUTEX_UNLOCKED = 0,
	// Mutex is currently locked.
	CMN_MUTEX_LOCKED,
	// Mutex has one or more waiting threads.
	CMN_MUTEX_WAITING,
} CmnMutexState;

// Lightweight mutex.
typedef struct CmnMutex {
	// Futex word storing the current CmnMutexState value.
	CmnFutex futex;
} CmnMutex;

// Internal slow path for mutex lock.
void cmnMutexLockSlow(CmnMutex* mutex, CmnMutexState initialState);

// Internal slow path for mutex unlock when waiters are present.
void cmnMutexUnlockSlow(CmnMutex* mutex);

// Lock a mutex, blocking if needed.
inline void cmnMutexLock(CmnMutex* mutex) {
	CmnMutexState state = (CmnMutexState)cmnAtomicExchange(&mutex->futex.value, (uint32_t)CMN_MUTEX_LOCKED, CMN_ACQUIRE);
	if (state != CMN_MUTEX_UNLOCKED) {
		cmnMutexLockSlow(mutex, state);
	}
}

// Try to lock a mutex without blocking.
inline bool cmnMutexTryLock(CmnMutex* mutex) {
	uint32_t expected = (uint32_t)CMN_MUTEX_UNLOCKED;
	return cmnAtomicCompareExchangeStrong(&mutex->futex.value, &expected, (uint32_t)CMN_MUTEX_LOCKED, CMN_ACQUIRE, CMN_ACQUIRE);
}

// Unlock a mutex.
inline void cmnMutexUnlock(CmnMutex* mutex) {
	CmnMutexState state = (CmnMutexState)cmnAtomicExchange(&mutex->futex.value, (uint32_t)CMN_MUTEX_UNLOCKED, CMN_RELEASE);
	if (state == CMN_MUTEX_WAITING) {
		// Some thread did wait with a futex for this unlock
		cmnMutexUnlockSlow(mutex);
	}
}

// RAII guard for CmnMutex.
typedef class CmnScopedMutex {
public:
	// Managed mutex.
	CmnMutex* mutex;

	// Lock mutex on construction.
	CmnScopedMutex(CmnMutex* mutex) {
		this->mutex = mutex;
		cmnMutexLock(this->mutex);
	}

	// Lock mutex on construction.
	CmnScopedMutex(const CmnMutex* mutex) {
		this->mutex = (CmnMutex*)mutex;
		cmnMutexLock(this->mutex);
	}

	// Unlock mutex on destruction.
	~CmnScopedMutex() {
		cmnMutexUnlock(this->mutex);
	}
} CmnScopedMutex;

#endif // CMN_MUTEX_H


