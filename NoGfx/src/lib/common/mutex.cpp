#include "mutex.h"

void cmnMutexLockSlow(CmnMutex* mutex, CmnMutexState initialState) {
	for (;;) {
		// NOTE: spin wait
		for (size_t spin = 0; spin < 128; spin++) {
			// NOTE: Let's try to acquire a lock. initialState can only be `CMN_MUTEX_LOCKED` or
			//	`CMN_MUTEX_WAITING`. It is worth noting that, if no thread in the futex-wait loop
			//	returns to the spin-wait loop, only one thread at the time can have initialState ==
			//	`CMN_MUTEX_WAITING`, thus minimizing the futex signals.
			uint32_t currentState = (uint32_t)CMN_MUTEX_UNLOCKED;
			bool didLock = cmnAtomicCompareExchangeWeak(&mutex->futex.value, &currentState, (uint32_t)initialState, CMN_ACQUIRE, CMN_ACQUIRE);
			if (didLock) {
				return;
			}

			// If someone is waiting with a futex, we all should wait with a futex
			if ((CmnMutexState)currentState == CMN_MUTEX_WAITING) {
				break;
			}

			for (size_t i = 0; i < spin; i++) {
				// TODO: make it not compiler and platform dependent
				// isb is better than yield and nop
				__builtin_arm_isb(0xF);
			}
		}

		// NOTE: If we are here, then the spin wait could not get a lock. Let's then wait with the futex.
		for (;;) {
			CmnMutexState currentState = (CmnMutexState)cmnAtomicExchange(&mutex->futex.value, (uint32_t)CMN_MUTEX_WAITING, CMN_ACQUIRE);
			if (currentState == CMN_MUTEX_UNLOCKED) {
				return;
			} else if (currentState == CMN_MUTEX_LOCKED) {
				// NOTE: It is possible that another thread tries to lock the mutex between the previous
				//	atomic exchange	and this futex wait. If this happens the mutex state should be
				//	CMN_MUTEX_UNLOCKED, thus it is possible to retry spin waiting. It is unoptimal,
				//	but we must retry with the initialState set to waiting.
				initialState = CMN_MUTEX_WAITING;
				break;
			}

			// NOTE: In the contention situation explained in the previous note occurs, the mutex state
			//	could be CMN_MUTEX_LOCKED, thus skipping a futex wait.
			cmnFutexWait(&mutex->futex, CMN_MUTEX_WAITING);
		}
	}
}

void cmnMutexUnlockSlow(CmnMutex* mutex) {
	cmnFutexSignal(&mutex->futex);
}


