#ifndef CMN_HANDLEMAP_H
#define CMN_HANDLEMAP_H

#include <limits.h>
#include <lib/common/common.h>
#include <lib/common/allocator.h>
#include <lib/common/exponential_array.h>

/**
	Opaque generational handle used to reference entries in CmnHandleMap.
	@see CmnHandleMap
*/
typedef struct CmnHandle {
	/** Bucket index. */
	uint32_t	index		: 32;
	/** Generation counter used to validate stale handles. */
	uint32_t	generation	: 32;
} CmnHandle;

/**
	Storage bucket used internally by CmnHandleMap.
	@see CmnHandleMap
*/
template <typename T>
struct CmnHandleMapBucket {
	/** True when the bucket stores a valid element. */
	bool		isInUse;
	/** Current generation of the bucket. */
	uint32_t	generation;
	union {
		// If isInUse
		/** Stored element when the bucket is in use. */
		T		element;
		// If not isInUse
		/** Next free bucket index when the bucket is free. */
		uint32_t	nextFreeIndex;
	};
};

/**
	Associative container using generational handles for stable references.
*/
template <typename T>
struct CmnHandleMap {
	/** Backing bucket storage. */
	CmnExponentialArray<CmnHandleMapBucket<T>>	buckets;
	/** Head index of the free bucket list. */
	uint32_t					firstFree;
	/** Value returned when handle lookup fails. */
	T						defaultElement;
};

/**
	Initializes a handle map.

	@param map The map to initialize.
	@param backingAllocator Allocator used for backing storage.
	@param defaultElement Value returned for invalid handle lookups.
	@param[out] result The result of the operation.
	@retval CMN_SUCCESS Handle map initialization completed successfully.
	@retval CMN_OUT_OF_MEMORY Backing storage allocation failed.
	@relates CmnHandleMap
*/
template <typename T>
void cmnCreateHandleMap(CmnHandleMap<T>* map, CmnAllocator backingAllocator, T defaultElement, CmnResult* result);

/**
	Gets the next free bucket index, extending storage when necessary.

	@param map The target map.
	@param[out] result The result of the operation.

	@return The index of a free bucket.
	@retval CMN_SUCCESS A free bucket index was produced successfully.
	@retval CMN_OUT_OF_MEMORY Backing storage growth failed.
	@relates CmnHandleMap
*/
template <typename T>
uint32_t cmnHandleMapGetNextFreeBucket(CmnHandleMap<T>* map, CmnResult* result);

/**
	Inserts an element and returns its handle.

	@param map The target map.
	@param element The element to insert.
	@param[out] result The result of the operation.

	@return The generated handle.
	@retval CMN_SUCCESS The insertion succeeded.
	@retval CMN_OUT_OF_MEMORY Backing storage growth failed.
	@relates CmnHandleMap
*/
template <typename T>
CmnHandle cmnInsert(CmnHandleMap<T>* map, const T& element, CmnResult* result);

/**
	Checks whether a handle is currently valid.

	@param map The target map.
	@param handle The handle to validate.

	@return True when the handle is valid, false otherwise.
	@relates CmnHandleMap
*/
template <typename T>
bool cmnIsValid(const CmnHandleMap<T>* map, CmnHandle handle);

/**
	Gets the element referenced by a handle.

	@param map The target map.
	@param handle The handle to resolve.
	@param[out] wasHandleValid Output flag reporting handle validity.

	@return Reference to the element when valid, or to `defaultElement` otherwise.
	@relates CmnHandleMap
*/
template <typename T>
T& cmnGet(CmnHandleMap<T>* map, CmnHandle handle, bool* wasHandleValid);

/**
	Removes the element referenced by a handle.

	@param map The target map.
	@param handle The handle to remove.
	@relates CmnHandleMap
*/
template <typename T>
void cmnRemove(CmnHandleMap<T>* map, CmnHandle handle);

#include "handle_map.inc"

#endif // CMN_HANDLEMAP_H

