#ifndef CMN_ATOMICCLANG_H
#define CMN_ATOMICCLANG_H

#include <lib/common/compiler.h>

#if defined(CMN_COMPILER_CLANG) || defined(CMN_COMPILER_GCC)

static const int gCmnMemoryOrderToGnu[] = {
	/*CMN_RELAXED*/	__ATOMIC_RELAXED,
	/*CMN_ACQUIRE*/ __ATOMIC_ACQUIRE,
	/*CMN_CONSUME*/ __ATOMIC_CONSUME,
	/*CMN_RELEASE*/ __ATOMIC_RELEASE,
	/*CMN_ACQ_REL*/ __ATOMIC_ACQ_REL,
	/*CMN_SEQ_CST*/ __ATOMIC_SEQ_CST,
};

template <typename T>
T cmnAtomicLoad(volatile T* ptr, CmnMemoryOrder order) {
	T ret;
	__ATOMIC_SEQ_CST;
	__atomic_load(ptr, &ret, gCmnMemoryOrderToGnu[order]);
	return ret;
}

template <typename T>
void cmnAtomicStore(volatile T* ptr, T value, CmnMemoryOrder order) {
	__atomic_store(ptr, &value, gCmnMemoryOrderToGnu[order]);
}

template <typename T>
T cmnAtomicExchange(volatile T* ptr, T value, CmnMemoryOrder order) {
	T ret;
	__atomic_exchange(ptr, &value, &ret, gCmnMemoryOrderToGnu[order]);
	return ret;
}

template <typename T>
bool cmnAtomicCompareExchangeStrong(volatile T* ptr, T expected, T value, CmnMemoryOrder order) {
	return __atomic_compare_exchange(
		ptr,
		&expected,
		&value,
		false, // strong
		gCmnMemoryOrderToGnu[order],
		gCmnMemoryOrderToGnu[order]
	);
}

template <typename T>
bool cmnAtomicCompareExchangeWeak(volatile T* ptr, T expected, T value, CmnMemoryOrder order) {
	return __atomic_compare_exchange(
		ptr,
		&expected,
		&value,
		true, // weak
		gCmnMemoryOrderToGnu[order],
		gCmnMemoryOrderToGnu[order]
	);
}

template <typename T>
void cmnAtomicAdd(volatile T* ptr, T value, CmnMemoryOrder order) {
	__atomic_fetch_add(ptr, value, gCmnMemoryOrderToGnu[order]);
}

template <typename T>
void cmnAtomicSub(volatile T* ptr, T value, CmnMemoryOrder order) {
	__atomic_fetch_sub(ptr, value, gCmnMemoryOrderToGnu[order]);
}

template <typename T>
void cmnAtomicAnd(volatile T* ptr, T value, CmnMemoryOrder order) {
	__atomic_fetch_and(ptr, value, gCmnMemoryOrderToGnu[order]);
}

template <typename T>
void cmnAtomicNand(volatile T* ptr, T value, CmnMemoryOrder order) {
	__atomic_fetch_nand(ptr, value, gCmnMemoryOrderToGnu[order]);
}

template <typename T>
void cmnAtomicOr(volatile T* ptr, T value, CmnMemoryOrder order) {
	__atomic_fetch_or(ptr, value, gCmnMemoryOrderToGnu[order]);
}

template <typename T>
void cmnAtomicXor(volatile T* ptr, T value, CmnMemoryOrder order) {
	__atomic_fetch_xor(ptr, value, gCmnMemoryOrderToGnu[order]);
}

template <typename T>
void cmnAtomicFence(CmnMemoryOrder order) {
	__atomic_thread_fence(gCmnMemoryOrderToGnu[order]);
}

#endif

#endif // CMN_ATOMICCLANG_H
