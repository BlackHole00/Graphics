#ifndef CMN_HANDLEMAP_H
#define CMN_HANDLEMAP_H

#include <limits.h>
#include <lib/common/common.h>
#include <lib/common/allocator.h>
#include <lib/common/exponential_array.h>

typedef struct CmnHandle {
	uint32_t	index		: 32;
	uint32_t	generation	: 32;
} CmnHandle;

template <typename T>
struct CmnHandleMapBucket {
	bool		isInUse;
	uint32_t	generation;
	union {
		// If isInUse
		T		element;
		// If not isInUse
		uint32_t	nextFreeIndex;
	};
};

template <typename T>
struct CmnHandleMap {
	CmnExponentialArray<CmnHandleMapBucket<T>>	buckets;
	uint32_t					firstFree;
	T						defaultElement;
};

template <typename T>
void cmnCreateHandleMap(CmnHandleMap<T>* map, CmnAllocator backingAllocator, T defaultElement, CmnResult* result);

template <typename T>
uint32_t cmnHandleMapGetNextFreeBucket(CmnHandleMap<T>* map, CmnResult* result);

template <typename T>
CmnHandle cmnInsert(CmnHandleMap<T>* map, const T& element, CmnResult* result);

template <typename T>
bool cmnIsValid(const CmnHandleMap<T>* map, CmnHandle handle);

template <typename T>
T& cmnGet(CmnHandleMap<T>* map, CmnHandle handle, bool* wasHandleValid);

template <typename T>
void cmnRemove(CmnHandleMap<T>* map, CmnHandle handle);

#include "handle_map.inc"

#endif // CMN_HANDLEMAP_H

