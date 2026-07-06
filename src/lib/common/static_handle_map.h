#ifndef CMN_STATIC_HANDLEMAP_H
#define CMN_STATIC_HANDLEMAP_H

#include <limits.h>
#include <lib/common/common.h>
#include <lib/common/handle_map.h>

// Fixed-size static variant of `CmnHandleMap` backed by a compile-time
// array of length `N`.
template <typename T, size_t N>
struct CmnStaticHandleMap {
	CmnHandleMapBucket<T> buckets[N];
	uint32_t firstFree;

	T defaultElement;
};

// Initializes a static handle map.
//
// Inputs:
// - map: Map to initialize.
// - defaultElement: Value returned for invalid handle lookups.
// - result: Optional operation result.
template <typename T, size_t N>
void cmnCreateStaticHandleMap(CmnStaticHandleMap<T, N>* map, T defaultElement);

// Inserts an element and returns its generated handle for the static map.
//
// Inputs:
// - map: Target map.
// - element: Element to insert.
// - result: Optional operation result.
//
// Results:
// - CMN_SUCCESS: Insert succeeded.
// - CMN_OUT_OF_RESOURCE_SLOTS: No more handle slots are available.
template <typename T, size_t N>
CmnHandle cmnInsert(CmnStaticHandleMap<T, N>* map, const T& element, CmnResult* result);

// Checks whether a handle is currently valid for the static map.
template <typename T, size_t N>
bool cmnIsValid(const CmnStaticHandleMap<T, N>* map, CmnHandle handle);

// Returns the element referenced by handle for the static map.
template <typename T, size_t N>
T& cmnGet(CmnStaticHandleMap<T, N>* map, CmnHandle handle, bool* wasHandleValid);

// Removes the element referenced by handle for the static map.
template <typename T, size_t N>
void cmnRemove(CmnStaticHandleMap<T, N>* map, CmnHandle handle);

#include "static_handle_map.inc"

#endif // CMN_STATIC_HANDLEMAP_H
