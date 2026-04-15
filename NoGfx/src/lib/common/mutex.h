#ifndef CMN_MUTEX_H
#define CMN_MUTEX_H

#include <lib/common/futex.h>
#include <lib/common/atomic.h>

/**
	Internal state values of CmnMutex.
*/
typedef enum CmnMutexState {
	/** The mutex is not held by any thread. */
	CMN_MUTEX_UNLOCKED = 0,
	/** The mutex is currently locked. */
	CMN_MUTEX_LOCKED,
	/** The mutex has one or more waiting threads. */
	CMN_MUTEX_WAITING,
} CmnMutexState;

/**
	A lightweight mutex.
*/
typedef struct CmnMutex {
	/** Futex word storing the current CmnMutexState value. */
	CmnFutex futex;
} CmnMutex;

/**
	Slow locking path used after a failed fast-path lock attempt.

	@param mutex The mutex to lock.
	@param initialState The state observed by the caller.
	@relates CmnMutex
	@remarks For internal use only. The user should call `cmnMutexLock` instead.
*/
void cmnMutexLockSlow(CmnMutex* mutex, CmnMutexState initialState);

/**
	Slow unlock path used when waiters are present.

	@param mutex The mutex to unlock.
	@relates CmnMutex
	@remarks For internal use only. The user should call `cmnMutexUnlock` instead.
*/
void cmnMutexUnlockSlow(CmnMutex* mutex);

/**
	Locks a mutex, blocking when needed.

	@param mutex The mutex to lock.
	@relates CmnMutex
*/
inline void cmnMutexLock(CmnMutex* mutex) {
	CmnMutexState state = (CmnMutexState)cmnAtomicExchange(&mutex->futex.value, (uint32_t)CMN_MUTEX_LOCKED, CMN_ACQUIRE);
	if (state != CMN_MUTEX_UNLOCKED) {
		cmnMutexLockSlow(mutex, state);
	}
}

/**
	Attempts to lock a mutex without blocking.

	@param mutex The mutex to lock.

	@return True when the lock was acquired, false otherwise.
	@relates CmnMutex
*/
inline bool cmnMutexTryLock(CmnMutex* mutex) {
	uint32_t expected = (uint32_t)CMN_MUTEX_UNLOCKED;
	return cmnAtomicCompareExchangeStrong(&mutex->futex.value, &expected, (uint32_t)CMN_MUTEX_LOCKED, CMN_ACQUIRE, CMN_ACQUIRE);
}

/**
	Unlocks a mutex.

	@param mutex The mutex to unlock.
	@relates CmnMutex
*/
inline void cmnMutexUnlock(CmnMutex* mutex) {
	CmnMutexState state = (CmnMutexState)cmnAtomicExchange(&mutex->futex.value, (uint32_t)CMN_MUTEX_UNLOCKED, CMN_RELEASE);
	if (state == CMN_MUTEX_WAITING) {
		// Some thread did wait with a futex for this unlock
		cmnMutexUnlockSlow(mutex);
	}
}

/**
	RAII guard that locks a mutex on construction and unlocks on destruction.
*/
typedef class CmnScopedMutex {
public:
	/** The managed mutex. */
	CmnMutex* mutex;

	/**
		Constructs a scoped guard and locks the mutex.

		@param mutex The mutex to lock.
		@relates CmnScopedMutex
	*/
	CmnScopedMutex(CmnMutex* mutex) {
		this->mutex = mutex;
		cmnMutexLock(mutex);
	}

	/**
		Destructs the guard and unlocks the mutex.
		@relates CmnScopedMutex
	*/
	~CmnScopedMutex() {
		cmnMutexUnlock(this->mutex);
	}
} CmnScopedMutex;

#endif // CMN_MUTEX_H


