#ifndef CMN_EXPONENTIALARRAY_H
#define CMN_EXPONENTIALARRAY_H

#include <lib/common/common.h>
#include <lib/common/arena.h>

#include <strings.h>
#include <assert.h>

#if CMN_ARCHITECTURE_BITS != 64
	#panic CmnExponentialArray requires a 64 bit architecture.
#endif

#define CMN_EXPONENTIALARRAY_BASE_BUCKET_LEN 32
#define CMN_EXPONENTIALARRAY_BASE_BUCKET_LEN_BITS 5
#define CMN_EXPONENTIALARRAY_POINTER_SIZE 64


template <typename T, size_t N>
struct CmnExponentialArray;


template <typename T, size_t N> void cmnCreateExponentialArray(CmnExponentialArray<T, N>* array, CmnArena* arena, CmnResult* result);
template <typename T, size_t N> bool cmnResize(CmnExponentialArray<T, N>* array, size_t length, CmnResult* result);
template <typename T, size_t N> void cmnSet(CmnExponentialArray<T, N>* array, size_t index, const T& value);
template <typename T, size_t N>   T& cmnGet(CmnExponentialArray<T, N>* array, size_t index);
template <typename T, size_t N> bool cmnAppend(CmnExponentialArray<T, N>* array, const T& value, CmnResult* result);

inline void cmnDecomposeExponentialArrayIndex(size_t index, size_t* bucketIndex, size_t* elementIndex);


template <typename T, size_t N = 15>
struct CmnExponentialArray {
	CmnArena* backingArena;

	T* buckets[N];
	size_t length;
	size_t last_filled_bucket;

	const T& operator[](size_t index) const {
		return cmnGet<T, N>(this, index);
	}

	T& operator[](size_t index) {
		return cmnGet(this, index);
	}
};

#include "exponential_array.inc"

#endif

