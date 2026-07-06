#include "condition.h"

void cmnConditionSignal(CmnCondition* condition) {
	cmnAtomicAdd(&condition->futex.value, 1u, CMN_RELEASE);
	cmnFutexSignal(&condition->futex);
}

void cmnConditionBroadcast(CmnCondition* condition) {
	cmnAtomicAdd(&condition->futex.value, 1u, CMN_RELEASE);
	cmnFutexBroadcast(&condition->futex);
}

void cmnConditionWait(CmnCondition* condition, CmnMutex* mutex) {
	uint32_t expected = cmnAtomicLoad(&condition->futex.value, CMN_ACQUIRE);

	cmnMutexUnlock(mutex);
	cmnFutexWait(&condition->futex, expected);
	cmnMutexLock(mutex);
}

bool cmnConditionWaitWithTimeout(CmnCondition* condition, CmnMutex* mutex, uint64_t ns) {
	uint32_t expected = cmnAtomicLoad(&condition->futex.value, CMN_ACQUIRE);

	cmnMutexUnlock(mutex);
	bool didWake = cmnFutexWaitWithTimeout(&condition->futex, expected, ns);
	cmnMutexLock(mutex);

	return didWake;
}
