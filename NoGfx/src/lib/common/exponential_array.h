#ifndef CMN_EXPONENTIALARRAY_H
#define CMN_EXPONENTIALARRAY_H

#include <lib/common/common.h>
#include <lib/common/allocator.h>

#include <strings.h>
#include <assert.h>

#if CMN_ARCHITECTURE_BITS != 64
	#panic CmnExponentialArray requires a 64 bit architecture.
#endif

/** Base bucket length used by CmnExponentialArray. */
#define CMN_EXPONENTIALARRAY_BASE_BUCKET_LEN 32
/** Number of bits used to represent the base bucket length. */
#define CMN_EXPONENTIALARRAY_BASE_BUCKET_LEN_BITS 5
/** Pointer size in bits assumed by the implementation. */
#define CMN_EXPONENTIALARRAY_POINTER_SIZE 64


template <typename T, size_t N>
struct CmnExponentialArray;


/**
	Initializes an exponential array.

	@param array The array to initialize.
	@param backingAllocator Allocator used for bucket allocations.
	@param[out] result The result of the operation.
	@retval CMN_SUCCESS Initialization completed successfully.
	@retval CMN_OUT_OF_MEMORY Backing bucket allocation failed.
	@relates CmnExponentialArray
*/
template <typename T, size_t N> void cmnCreateExponentialArray(CmnExponentialArray<T, N>* array, CmnAllocator backingAllocator, CmnResult* result);

/**
	Resizes an exponential array to a new logical length.

	@param array The array to resize.
	@param length The new logical length.
	@param[out] result The result of the operation.

	@return True on success, false otherwise.
	@retval CMN_SUCCESS Resize completed successfully.
	@retval CMN_OUT_OF_MEMORY Requested growth exceeds available buckets or allocation failed.
	@relates CmnExponentialArray
*/
template <typename T, size_t N> bool cmnResize(CmnExponentialArray<T, N>* array, size_t length, CmnResult* result);

/**
	Sets the value at a specific index.

	@param array The target array.
	@param index The destination index.
	@param value The value to store.
	@relates CmnExponentialArray
*/
template <typename T, size_t N> void cmnSet(CmnExponentialArray<T, N>* array, size_t index, const T& value);

/**
	Gets the value at a specific index.

	@param array The target array.
	@param index The source index.

	@return Reference to the stored value.
	@relates CmnExponentialArray
*/
template <typename T, size_t N>   T& cmnGet(CmnExponentialArray<T, N>* array, size_t index);

/**
	Appends a value at the end of the array.

	@param array The target array.
	@param value The value to append.
	@param[out] result The result of the operation.

	@return True on success, false otherwise.
	@retval CMN_SUCCESS Append completed successfully.
	@retval CMN_OUT_OF_MEMORY Append failed due to capacity or allocation limits.
	@relates CmnExponentialArray
*/
template <typename T, size_t N> bool cmnAppend(CmnExponentialArray<T, N>* array, const T& value, CmnResult* result);

/**
	Gets a reference to the last logical element in the array.

	@param array The target array.

	@return Reference to the last element.
	@relates CmnExponentialArray
*/
template <typename T, size_t N> T& cmnLast(CmnExponentialArray<T, N>* array);

/**
	Removes the last logical element from the array.

	@param array The target array.
	@relates CmnExponentialArray
*/
template <typename T, size_t N> void cmnPop(CmnExponentialArray<T, N>* array);

/**
	Decomposes a linear index into bucket and element indices.

	@param index The linear index.
	@param[out] bucketIndex Output bucket index.
	@param[out] elementIndex Output index within the bucket.
*/
inline void cmnDecomposeExponentialArrayIndex(size_t index, size_t* bucketIndex, size_t* elementIndex);


/**
	Array-like container backed by exponentially growing buckets.
*/
template <typename T, size_t N = 15>
struct CmnExponentialArray {
	/** Allocator used for bucket allocations. */
	CmnAllocator	backingAllocator;

	/** Bucket pointers storing the array elements. */
	T* buckets[N];
	/** Current logical element count. */
	size_t length;
	/** Index of the last bucket containing at least one element. */
	size_t last_filled_bucket;

	/**
		Gets a constant element reference by index.

		@param index The source index.

		@return Constant reference to the element.
	*/
	const T& operator[](size_t index) const {
		return cmnGet<T, N>((CmnExponentialArray<T, N>*)this, index);
	}

	/**
		Gets a mutable element reference by index.

		@param index The source index.

		@return Mutable reference to the element.
	*/
	T& operator[](size_t index) {
		return cmnGet<T, N>(this, index);
	}
};

#include "exponential_array.inc"

#endif

