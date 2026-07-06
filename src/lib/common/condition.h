#ifndef CMN_CONDITION_H
#define CMN_CONDITION_H

#include <lib/common/mutex.h>

// Lightweight condition variable.
typedef struct CmnCondition {
	// Sequence value incremented on each signal or broadcast.
	CmnFutex futex;
} CmnCondition;

// Wake one thread waiting on the condition.
void cmnConditionSignal(CmnCondition* condition);

// Wake all threads waiting on the condition.
void cmnConditionBroadcast(CmnCondition* condition);

// Wait on a condition variable while releasing and reacquiring mutex.
void cmnConditionWait(CmnCondition* condition, CmnMutex* mutex);

// Wait on a condition variable with timeout.
bool cmnConditionWaitWithTimeout(CmnCondition* condition, CmnMutex* mutex, uint64_t ns);

#endif // CMN_CONDITION_H
