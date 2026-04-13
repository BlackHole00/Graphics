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
template <typename T> T cmnAtomicLoad(T* ptr, CmnMemoryOrder order = CMN_SEQ_CST);
template <typename T> void cmnAtomicStore(T* ptr, T value, CmnMemoryOrder order = CMN_SEQ_CST);
template <typename T> T cmnAtomicExchange(T* ptr, T value, CmnMemoryOrder order = CMN_SEQ_CST);
template <typename T> bool cmnAtomicCompareExchangeStrong(T* ptr, T expected, T value, CmnMemoryOrder successOrder = CMN_SEQ_CST, CmnMemoryOrder failureOrder = CMN_SEQ_CST);
template <typename T> bool cmnAtomicCompareExchangeWeak(T* ptr, T expected, T value, CmnMemoryOrder successOrder = CMN_SEQ_CST, CmnMemoryOrder failureOrder = CMN_SEQ_CST);
template <typename T> bool cmnAtomicCompareExchangeStrong(T* ptr, T* expected, T value, CmnMemoryOrder successOrder = CMN_SEQ_CST, CmnMemoryOrder failureOrder = CMN_SEQ_CST);
template <typename T> bool cmnAtomicCompareExchangeWeak(T* ptr, T* expected, T value, CmnMemoryOrder successOrder = CMN_SEQ_CST, CmnMemoryOrder failureOrder = CMN_SEQ_CST);
template <typename T> void cmnAtomicAdd(T* ptr, T value, CmnMemoryOrder order = CMN_SEQ_CST);
template <typename T> void cmnAtomicSub(T* ptr, T value, CmnMemoryOrder order = CMN_SEQ_CST);
template <typename T> void cmnAtomicAnd(T* ptr, T value, CmnMemoryOrder order = CMN_SEQ_CST);
template <typename T> void cmnAtomicNand(T* ptr, T value, CmnMemoryOrder order = CMN_SEQ_CST);
template <typename T> void cmnAtomicOr(T* ptr, T value, CmnMemoryOrder order = CMN_SEQ_CST);
template <typename T> void cmnAtomicXor(T* ptr, T value, CmnMemoryOrder order = CMN_SEQ_CST);

template <typename T> void cmnAtomicFence(CmnMemoryOrder order);

#include "atomic_gnu.inc"

#endif // CMN_ATOMIC_H

