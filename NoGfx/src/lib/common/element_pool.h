#ifndef CMN_ELEMENTPOOL_H
#define CMN_ELEMENTPOOL_H

#include <limits.h>
#include <lib/common/exponential_array.h>

template <typename T>
struct CmnElementPool;

// cmnCreateElementPool initializes an element pool.
//
// Inputs:
// - pool: Pool to initialize.
// - backingAllocator: Allocator used for pool backing storage.
// - result: Optional operation result.
//
// Results:
// - CMN_SUCCESS: Initialization succeeded.
// - CMN_OUT_OF_MEMORY: Backing allocator ran out of memory.
template <typename T>
void cmnCreateElementPool(CmnElementPool<T>* pool, CmnAllocator backingAllocator, CmnResult* result);

// cmnInsert inserts element into the pool and returns its slot index.
// Free slots are reused before extending storage.
//
// Inputs:
// - pool: Target pool.
// - element: Element value to insert.
// - result: Optional operation result.
//
// Results:
// - CMN_SUCCESS: Insert succeeded.
// - CMN_OUT_OF_MEMORY: Backing allocator ran out of memory while growing storage.
//
// Returns:
// - Slot index on success, SIZE_MAX on failure.
template <typename T>
size_t cmnInsert(CmnElementPool<T>* pool, const T& element, CmnResult* result);

// cmnRemove removes the element at index and marks the slot reusable.
//
// Inputs:
// - pool: Target pool.
// - index: Slot index to remove.
template <typename T>
void cmnRemove(CmnElementPool<T>* pool, size_t index);

// cmnGet returns a mutable reference to the element at index.
//
// Inputs:
// - pool: Source pool.
// - index: Slot index.
//
// Returns:
// - Mutable reference to the stored element.
template <typename T>
T& cmnGet(CmnElementPool<T>* pool, size_t index);

// cmnSet overwrites an element at index.
//
// Inputs:
// - pool: Target pool.
// - index: Destination slot index.
// - element: Value to write.
template <typename T>
void cmnSet(CmnElementPool<T>* pool, size_t index, const T& element);

// Sparse element pool with free-slot reuse.
template <typename T>
struct CmnElementPool {
	// Number of active logical elements.
	size_t length;

	CmnExponentialArray<T> elements;
	CmnExponentialArray<size_t> freeList;

	const T& operator[](size_t index) const {
		return cmnGet<T>((CmnElementPool<T>*)this, index);
	}

	T& operator[](size_t index) {
		return cmnGet<T>(this, index);
	}
};

#include "element_pool.inc"

#endif // CMN_ELEMENTPOOL_H
