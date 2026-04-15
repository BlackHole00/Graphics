#ifndef CMN_ATOMIC_H
#define CMN_ATOMIC_H

/**
	Memory ordering constraints for atomic operations.
*/
typedef enum CmnMemoryOrder {
	/** No ordering constraints are enforced. */
	CMN_RELAXED,
	/** Acquire ordering for subsequent memory operations. */
	CMN_ACQUIRE,
	/** Consume ordering for dependent memory operations. */
	CMN_CONSUME,
	/** Release ordering for prior memory operations. */
	CMN_RELEASE,
	/** Combined acquire and release ordering. */
	CMN_ACQ_REL,
	/** Sequentially consistent ordering. */
	CMN_SEQ_CST
} CmnMemoryOrder;

/**
	Atomically loads a value.

	@param ptr Pointer to the atomic storage.
	@param order Memory order to apply to the operation.

	@return The loaded value.
	@remark Compiler specific.
*/
template <typename T> T cmnAtomicLoad(T* ptr, CmnMemoryOrder order = CMN_SEQ_CST);

/**
	Atomically stores a value.

	@param ptr Pointer to the atomic storage.
	@param value Value to store.
	@param order Memory order to apply to the operation.

	@remark Compiler specific.
*/
template <typename T> void cmnAtomicStore(T* ptr, T value, CmnMemoryOrder order = CMN_SEQ_CST);

/**
	Atomically exchanges a value and returns the old one.

	@param ptr Pointer to the atomic storage.
	@param value New value to store.
	@param order Memory order to apply to the operation.

	@return The previous value.
	@remark Compiler specific.
*/
template <typename T> T cmnAtomicExchange(T* ptr, T value, CmnMemoryOrder order = CMN_SEQ_CST);

/**
	Performs a strong compare-and-exchange using value expected by copy.

	@param ptr Pointer to the atomic storage.
	@param expected Expected current value.
	@param value Value written on success.
	@param successOrder Memory order used on success.
	@param failureOrder Memory order used on failure.

	@return True when the exchange succeeds, false otherwise.
	@remark Compiler specific.
*/
template <typename T> bool cmnAtomicCompareExchangeStrong(T* ptr, T expected, T value, CmnMemoryOrder successOrder = CMN_SEQ_CST, CmnMemoryOrder failureOrder = CMN_SEQ_CST);

/**
	Performs a weak compare-and-exchange using value expected by copy.

	@param ptr Pointer to the atomic storage.
	@param expected Expected current value.
	@param value Value written on success.
	@param successOrder Memory order used on success.
	@param failureOrder Memory order used on failure.

	@return True when the exchange succeeds, false otherwise.
	@remark Compiler specific.
*/
template <typename T> bool cmnAtomicCompareExchangeWeak(T* ptr, T expected, T value, CmnMemoryOrder successOrder = CMN_SEQ_CST, CmnMemoryOrder failureOrder = CMN_SEQ_CST);

/**
	Performs a strong compare-and-exchange using expected by pointer.

	@param ptr Pointer to the atomic storage.
	@param expected[inout] Pointer to the expected value, updated on failure.
	@param value Value written on success.
	@param successOrder Memory order used on success.
	@param failureOrder Memory order used on failure.

	@return True when the exchange succeeds, false otherwise.
	@remark Compiler specific.
*/
template <typename T> bool cmnAtomicCompareExchangeStrong(T* ptr, T* expected, T value, CmnMemoryOrder successOrder = CMN_SEQ_CST, CmnMemoryOrder failureOrder = CMN_SEQ_CST);

/**
	Performs a weak compare-and-exchange using expected by pointer.

	@param ptr Pointer to the atomic storage.
	@param expected[inout] Pointer to the expected value, updated on failure.
	@param value Value written on success.
	@param successOrder Memory order used on success.
	@param failureOrder Memory order used on failure.

	@return True when the exchange succeeds, false otherwise.
	@remark Compiler specific.
*/
template <typename T> bool cmnAtomicCompareExchangeWeak(T* ptr, T* expected, T value, CmnMemoryOrder successOrder = CMN_SEQ_CST, CmnMemoryOrder failureOrder = CMN_SEQ_CST);

/**
	Atomically adds a value.

	@param ptr Pointer to the atomic storage.
	@param value Value to add.
	@param order Memory order to apply to the operation.

	@remark Compiler specific.
*/
template <typename T> void cmnAtomicAdd(T* ptr, T value, CmnMemoryOrder order = CMN_SEQ_CST);

/**
	Atomically subtracts a value.

	@param ptr Pointer to the atomic storage.
	@param value Value to subtract.
	@param order Memory order to apply to the operation.

	@remark Compiler specific.
*/
template <typename T> void cmnAtomicSub(T* ptr, T value, CmnMemoryOrder order = CMN_SEQ_CST);

/**
	Atomically applies bitwise AND.

	@param ptr Pointer to the atomic storage.
	@param value Value used for the operation.
	@param order Memory order to apply to the operation.

	@remark Compiler specific.
*/
template <typename T> void cmnAtomicAnd(T* ptr, T value, CmnMemoryOrder order = CMN_SEQ_CST);

/**
	Atomically applies bitwise NAND.

	@param ptr Pointer to the atomic storage.
	@param value Value used for the operation.
	@param order Memory order to apply to the operation.

	@remark Compiler specific.
*/
template <typename T> void cmnAtomicNand(T* ptr, T value, CmnMemoryOrder order = CMN_SEQ_CST);

/**
	Atomically applies bitwise OR.

	@param ptr Pointer to the atomic storage.
	@param value Value used for the operation.
	@param order Memory order to apply to the operation.

	@remark Compiler specific.
*/
template <typename T> void cmnAtomicOr(T* ptr, T value, CmnMemoryOrder order = CMN_SEQ_CST);

/**
	Atomically applies bitwise XOR.

	@param ptr Pointer to the atomic storage.
	@param value Value used for the operation.
	@param order Memory order to apply to the operation.

	@remark Compiler specific.
*/
template <typename T> void cmnAtomicXor(T* ptr, T value, CmnMemoryOrder order = CMN_SEQ_CST);

/**
	Issues an atomic thread fence.

	@param order Memory order to apply to the fence.

	@remark Compiler specific.
*/
template <typename T> void cmnAtomicFence(CmnMemoryOrder order);

#include "atomic_gnu.inc"

#endif // CMN_ATOMIC_H

