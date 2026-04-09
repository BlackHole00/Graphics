#ifndef CMN_MUTEX_H
#define CMN_MUTEX_H

#include <lib/common/common.h>

typedef uint32_t CmnFutex;

// NOTE: Platform dependent
void cmnFutexBroadcast(CmnFutex* futex);
void cmnFutexSignal(CmnFutex* futex);
void cmnFutexWait(CmnFutex* futex, uint32_t expected);
bool cmnFutexWaitWithTimeout(CmnFutex* futex, uint32_t expected, uint64_t ns);

#include "futex_darwin.inc"

#endif // CMN_MUTEX_H

