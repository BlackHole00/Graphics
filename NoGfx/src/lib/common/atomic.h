#ifndef CMN_ATOMIC_H
#define CMN_ATOMIC_H

typedef enum CmnMemoryOrder {
	CMN_RELAXED,
	CMN_ACQUIRE,
	CMN_CONSUME,
	CMN_RELEASE,
	CMN_ACQ_REL,
	CMN_SEQ_CST
} CmnMemoryOrder;

// NOTE: Compiler dependent
template <typename T> T cmnAtomicLoad(volatile T* ptr, CmnMemoryOrder order = CMN_RELAXED);
template <typename T> void cmnAtomicStore(volatile T* ptr, T value, CmnMemoryOrder order = CMN_RELAXED);
template <typename T> T cmnAtomicExchange(volatile T* ptr, T value, CmnMemoryOrder order = CMN_RELAXED);
template <typename T> bool cmnAtomicCompareExchangeStrong(volatile T* ptr, T expected, T value, CmnMemoryOrder order = CMN_RELAXED);
template <typename T> bool cmnAtomicCompareExchangeWeak(volatile T* ptr, T expected, T value, CmnMemoryOrder order = CMN_RELAXED);
template <typename T> void cmnAtomicAdd(volatile T* ptr, T value, CmnMemoryOrder order = CMN_RELAXED);
template <typename T> void cmnAtomicSub(volatile T* ptr, T value, CmnMemoryOrder order = CMN_RELAXED);
template <typename T> void cmnAtomicAnd(volatile T* ptr, T value, CmnMemoryOrder order = CMN_RELAXED);
template <typename T> void cmnAtomicNand(volatile T* ptr, T value, CmnMemoryOrder order = CMN_RELAXED);
template <typename T> void cmnAtomicOr(volatile T* ptr, T value, CmnMemoryOrder order = CMN_RELAXED);
template <typename T> void cmnAtomicXor(volatile T* ptr, T value, CmnMemoryOrder order = CMN_RELAXED);

template <typename T> void cmnAtomicFence(CmnMemoryOrder order);

#include "atomic_gnu.inc"

#endif // CMN_ATOMIC_H

