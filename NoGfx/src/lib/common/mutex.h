#ifndef CMN_MUTEX_H
#define CMN_MUTEX_H

#include <lib/common/futex.h>
#include <lib/common/atomic.h>

typedef enum CmnMutexState {
	CMN_MUTEX_UNLOCKED,
	CMN_MUTEX_LOCKED,
	CMN_MUTEX_WAITING,
} CmnMutexState;

typedef CmnMutexState CmnMutex;

void cmnMutexLockSlow(CmnMutex* mutex, CmnMutexState initialState);
void cmnMutexUnlockSlow(CmnMutex* mutex);

inline void cmnMutexLock(CmnMutex* mutex) {
	CmnMutexState state = cmnAtomicExchange(mutex, CMN_MUTEX_LOCKED, CMN_ACQUIRE);
	if (state != CMN_MUTEX_UNLOCKED) {
		cmnMutexLockSlow(mutex, state);
	}
}

inline bool cmnMutexTryLock(CmnMutex* mutex) {
	return cmnAtomicCompareExchangeStrong(mutex, CMN_MUTEX_UNLOCKED, CMN_MUTEX_LOCKED, CMN_ACQUIRE, CMN_ACQUIRE);
}

inline void cmnMutexUnlock(CmnMutex* mutex) {
	CmnMutexState state = cmnAtomicExchange(mutex, CMN_MUTEX_UNLOCKED, CMN_RELEASE);
	if (state == CMN_MUTEX_WAITING) {
		// Some thread did wait with a futex for this unlock
		cmnMutexUnlockSlow(mutex);
	}
}

typedef class CmnScopedMutex {
public:
	CmnMutex* mutex;

	CmnScopedMutex(CmnMutex* mutex) {
		this->mutex = mutex;
		cmnMutexLock(mutex);
	}

	~CmnScopedMutex() {
		cmnMutexUnlock(this->mutex);
	}
} CmnScopedMutex;

#endif // CMN_MUTEX_H


