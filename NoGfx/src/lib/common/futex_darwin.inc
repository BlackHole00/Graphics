#include <lib/common/platform.h>

#ifdef CMN_PLATFORM_DARWIN

#include <assert.h>
#include <sys/errno.h>
#include <os/os_sync_wait_on_address.h>

#include <lib/common/atomic.h>

inline void cmnFutexBroadcast(CmnFutex* futex) {
	int res = os_sync_wake_by_address_all(futex, 4, OS_SYNC_WAKE_BY_ADDRESS_NONE);
	assert(res >= 0 && "os_sync_wait_on_address_with_timeout error.");
}

inline void cmnFutexSignal(CmnFutex* futex) {
	int res = os_sync_wake_by_address_any(futex, 4, OS_SYNC_WAKE_BY_ADDRESS_NONE);
	assert(res >= 0 && "os_sync_wait_on_address_with_timeout error.");
}

inline void cmnFutexWait(CmnFutex* futex, uint32_t expected) {
	for (;;) {
		if (cmnAtomicLoad(futex, CMN_ACQUIRE) != expected) {
			break;
		}

		int res = os_sync_wait_on_address(
			futex,
			expected,
			4,
			OS_SYNC_WAIT_ON_ADDRESS_NONE
		);

		if (res < 0 && (errno == EINTR || errno == EFAULT)) {
			continue;
		} else if (res >= 0) {
			return;
		} else {
			assert(false && "os_sync_wait_on_address_with_timeout error.");
		}
	}
}

inline bool cmnFutexWaitWithTimeout(CmnFutex* futex, uint32_t expected, uint64_t ns) {
	if (ns <= 0) {
		return false;
	}

	for (;;) {
		if (cmnAtomicLoad(futex, CMN_ACQUIRE) != expected) {
			break;
		}

		int res = os_sync_wait_on_address_with_timeout(
			futex,
			expected,
			4,
			OS_SYNC_WAIT_ON_ADDRESS_NONE,
			OS_CLOCK_MACH_ABSOLUTE_TIME,
			ns
		);

		if (res < 0 && (errno == EINTR || errno == EFAULT)) {
			continue;
		} else if (res < 0 && errno == ETIMEDOUT) {
			return false;
		} else if (res >= 0) {
			return true;
		} else {
			assert(false && "os_sync_wait_on_address_with_timeout error.");
		}
	}

	return false;
}

#endif

