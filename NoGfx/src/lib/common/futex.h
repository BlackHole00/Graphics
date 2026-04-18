#ifndef CMN_FUTEX_H
#define CMN_FUTEX_H

#include <lib/common/common.h>

// Futex value used by low-level wait and wake operations.
typedef struct CmnFutex {
	// Atomic word used by platform wait/wake primitives.
	uint32_t value;
} CmnFutex;

// Wake all threads waiting on futex.
void cmnFutexBroadcast(CmnFutex* futex);

// Wake one thread waiting on futex.
void cmnFutexSignal(CmnFutex* futex);

// Wait while futex equals expected.
void cmnFutexWait(CmnFutex* futex, uint32_t expected);

// Wait while futex equals expected, with timeout.
bool cmnFutexWaitWithTimeout(CmnFutex* futex, uint32_t expected, uint64_t ns);

#endif // CMN_FUTEX_H

