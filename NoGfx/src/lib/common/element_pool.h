#ifndef CMN_ELEMENTPOOL_H
#define CMN_ELEMENTPOOL_H

#include <limits.h>
#include <lib/common/exponential_array.h>

template <typename T>
struct CmnElementPool;

/**
	Initializes an element pool.

	@param pool The pool to initialize.
	@param backingAllocator Allocator used for pool backing storage.
	@param[out] result The result of the operation.
	@retval CMN_SUCCESS Initialization completed successfully.
	@retval CMN_OUT_OF_MEMORY Backing storage allocation failed.
	@relates CmnElementPool
*/
template <typename T>
void cmnCreateElementPool(CmnElementPool<T>* pool, CmnAllocator backingAllocator, CmnResult* result);

/**
	Inserts an element into the pool and returns its slot index.

	If there are free slots available, one is reused; otherwise a new slot is appended.

	@param pool The target pool.
	@param element The element value to insert.
	@param[out] result The result of the operation.

	@return Slot index on success, SIZE_T_MAX on failure.
	@retval CMN_SUCCESS Insertion completed successfully.
	@retval CMN_OUT_OF_MEMORY Growing the backing storage failed.
	@relates CmnElementPool
*/
template <typename T>
size_t cmnInsert(CmnElementPool<T>* pool, const T& element, CmnResult* result);

/**
	Removes the element at an index and marks the slot as reusable.

	If index is outside the allocated element range, the operation is ignored.

	@param pool The target pool.
	@param index The slot index to remove.
	@relates CmnElementPool
*/
template <typename T>
void cmnRemove(CmnElementPool<T>* pool, size_t index);

/**
	Gets a mutable reference to an element stored in the pool.

	@param pool The source pool.
	@param index The slot index.

	@return Mutable reference to the stored element.
	@relates CmnElementPool
*/
template <typename T>
T& cmnGet(CmnElementPool<T>* pool, size_t index);

/**
	Overwrites an element at a specific slot index.

	If index is outside the allocated element range, the operation is ignored.

	@param pool The target pool.
	@param index The destination slot index.
	@param element The value to write.
	@relates CmnElementPool
*/
template <typename T>
void cmnSet(CmnElementPool<T>* pool, size_t index, const T& element);

/**
	Sparse pool storing elements in an exponential array with a free-slot list.
*/
template <typename T>
struct CmnElementPool {
	/** Number of active logical elements tracked by the pool. */
	size_t length;

	/** Backing storage for all allocated slots. */
	CmnExponentialArray<T> elements;
	/** Indices of reusable slots. */
	CmnExponentialArray<size_t> freeList;

	/**
		Gets a constant element reference by slot index.

		@param index The source slot index.

		@return Constant reference to the stored element.
	*/
	const T& operator[](size_t index) const {
		return cmnGet<T>((CmnElementPool<T>*)this, index);
	}

	/**
		Gets a mutable element reference by slot index.

		@param index The source slot index.

		@return Mutable reference to the stored element.
	*/
	T& operator[](size_t index) {
		return cmnGet<T>(this, index);
	}
};

#include "element_pool.inc"

#endif // CMN_ELEMENTPOOL_H
