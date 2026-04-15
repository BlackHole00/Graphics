#ifndef CMN_FUTEX_H
#define CMN_FUTEX_H

#include <lib/common/common.h>

/**
	Futex integer type used for low-level thread synchronization.
*/
typedef struct CmnFutex {
	/** Atomic futex word used by platform wait/wake primitives. */
	uint32_t value;
} CmnFutex;

/**
	Wakes all threads waiting on the futex.

	@param futex The futex to signal.
	@remark Platform specific.
	@relates CmnFutex
*/
void cmnFutexBroadcast(CmnFutex* futex);

/**
	Wakes a single thread waiting on the futex.

	@param futex The futex to signal.
	@remark Platform specific.
	@relates CmnFutex
*/
void cmnFutexSignal(CmnFutex* futex);

/**
	Waits while the futex value matches `expected`.

	@param futex The futex to wait on.
	@param expected The expected futex value.
	@remark Platform specific.
	@relates CmnFutex
*/
void cmnFutexWait(CmnFutex* futex, uint32_t expected);

/**
	Waits while the futex value matches `expected`, with timeout.

	@param futex The futex to wait on.
	@param expected The expected futex value.
	@param ns Timeout in nanoseconds.

	@return True if signaled before timeout, false on timeout.
	@remark Platform specific.
	@relates CmnFutex
*/
bool cmnFutexWaitWithTimeout(CmnFutex* futex, uint32_t expected, uint64_t ns);

#endif // CMN_FUTEX_H

