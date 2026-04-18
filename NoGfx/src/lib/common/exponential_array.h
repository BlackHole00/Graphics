#ifndef CMN_EXPONENTIALARRAY_H
#define CMN_EXPONENTIALARRAY_H

#include <lib/common/common.h>
#include <lib/common/allocator.h>

#include <strings.h>
#include <assert.h>

#if CMN_ARCHITECTURE_BITS != 64
	#panic CmnExponentialArray requires a 64 bit architecture.
#endif

// Base bucket length used by CmnExponentialArray.
#define CMN_EXPONENTIALARRAY_BASE_BUCKET_LEN 32
// Number of bits used to represent the base bucket length.
#define CMN_EXPONENTIALARRAY_BASE_BUCKET_LEN_BITS 5
// Pointer size in bits assumed by the implementation.
#define CMN_EXPONENTIALARRAY_POINTER_SIZE 64


template <typename T, size_t N>
struct CmnExponentialArray;


// Initializes an exponential array.
//
// Inputs:
// - array: Array to initialize.
// - backingAllocator: Allocator used for bucket allocations.
// - result: Optional operation result.
//
// Results:
// - CMN_SUCCESS: Initialization succeeded.
// - CMN_OUT_OF_MEMORY: Backing allocator ran out of memory.
template <typename T, size_t N> void cmnCreateExponentialArray(CmnExponentialArray<T, N>* array, CmnAllocator backingAllocator, CmnResult* result);

// Changes the logical length of an exponential array.
//
// Inputs:
// - array: Array to resize.
// - length: New logical length.
// - result: Optional operation result.
//
// Results:
// - CMN_SUCCESS: Resize succeeded.
// - CMN_OUT_OF_MEMORY: Backing allocator ran out of memory.
//
// Returns:
// - true on success.
template <typename T, size_t N> bool cmnResize(CmnExponentialArray<T, N>* array, size_t length, CmnResult* result);

// Writes value at index.
//
// Inputs:
// - array: Target array.
// - index: Destination index.
// - value: Value to store.
template <typename T, size_t N> void cmnSet(CmnExponentialArray<T, N>* array, size_t index, const T& value);

// Returns the value reference at index.
//
// Inputs:
// - array: Target array.
// - index: Source index.
//
// Returns:
// - Mutable reference to stored value.
template <typename T, size_t N>   T& cmnGet(CmnExponentialArray<T, N>* array, size_t index);

// Appends value at the end of the array.
//
// Inputs:
// - array: Target array.
// - value: Value to append.
// - result: Optional operation result.
//
// Results:
// - CMN_SUCCESS: Append succeeded.
// - CMN_OUT_OF_MEMORY: Backing allocator ran out of memory.
//
// Returns:
// - true on success.
template <typename T, size_t N> bool cmnAppend(CmnExponentialArray<T, N>* array, const T& value, CmnResult* result);

// Returns a reference to the last logical element.
//
// Inputs:
// - array: Target array.
//
// Returns:
// - Mutable reference to the last element.
template <typename T, size_t N> T& cmnLast(CmnExponentialArray<T, N>* array);

// Removes the last logical element.
//
// Inputs:
// - array: Target array.
template <typename T, size_t N> void cmnPop(CmnExponentialArray<T, N>* array);

// Maps a linear index to bucket and element indices.
//
// Inputs:
// - index: Linear index.
// - bucketIndex: Output bucket index.
// - elementIndex: Output index within bucket.
inline void cmnDecomposeExponentialArrayIndex(size_t index, size_t* bucketIndex, size_t* elementIndex);


// Array-like container backed by exponentially growing buckets.
template <typename T, size_t N = 15>
struct CmnExponentialArray {
	CmnAllocator	backingAllocator;

	T* buckets[N];
	// Current logical element count.
	size_t length;
	size_t last_filled_bucket;

	const T& operator[](size_t index) const {
		return cmnGet<T, N>((CmnExponentialArray<T, N>*)this, index);
	}

	T& operator[](size_t index) {
		return cmnGet<T, N>(this, index);
	}
};

#include "exponential_array.inc"

#endif

