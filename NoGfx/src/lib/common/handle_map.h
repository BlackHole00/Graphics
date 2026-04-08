#ifndef CMN_HANDLEMAP_H
#define CMN_HANDLEMAP_H

#include <limits.h>
#include <lib/common/common.h>
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
void cmnCreateHandleMap(CmnHandleMap<T>* map, CmnArena* backingArena, T defaultElement, CmnResult* result) {
	CmnResult localResult;

	cmnCreateExponentialArray(&map->buckets, backingArena, &localResult);
	if (localResult != CMN_SUCCESS) {
		CMN_SET_RESULT(result, localResult);
		return;
	}

	CmnHandleMapBucket<T> firstBucket;
	firstBucket.isInUse		= false,
	firstBucket.generation		= 0,
	firstBucket.nextFreeIndex	= 1,

	cmnAppend(&map->buckets, firstBucket, &localResult);
	if (localResult != CMN_SUCCESS) {
		CMN_SET_RESULT(result, localResult);
		return;
	}

	map->firstFree = 0;
	map->defaultElement = defaultElement;

	CMN_SET_RESULT(result, CMN_SUCCESS);
}

template <typename T>
uint32_t cmnHandleMapGetNextFreeBucket(CmnHandleMap<T>* map, CmnResult* result) {
	CmnResult localResult;

	uint32_t firstFreeBucketIndex = map->firstFree;
	if (firstFreeBucketIndex == UINT32_MAX) {
		CMN_SET_RESULT(result, CMN_OUT_OF_RESOURCE_SLOTS);
		return UINT32_MAX;
	}

	for (;;) {
		if (firstFreeBucketIndex >= map->buckets.length) {
			break;
		}

		CmnHandleMapBucket<T>* bucket = &map->buckets[firstFreeBucketIndex];

		if (bucket->generation == UINT32_MAX) {
			firstFreeBucketIndex = bucket->nextFreeIndex;
		} else {
			break;
		}
	}


	if (firstFreeBucketIndex >= map->buckets.length) {
		firstFreeBucketIndex = map->buckets.length;

		CmnHandleMapBucket<T> newBucket;
		newBucket.isInUse	= false,
		newBucket.generation	= 0,
		newBucket.nextFreeIndex	= firstFreeBucketIndex + 1,

		cmnAppend(&map->buckets, newBucket, &localResult);
		if (localResult != CMN_SUCCESS) {
			CMN_SET_RESULT(result, localResult);
			return UINT32_MAX;
		}
	}

	CmnHandleMapBucket<T>* bucket = &map->buckets[firstFreeBucketIndex];
	map->firstFree = bucket->nextFreeIndex;

	CMN_SET_RESULT(result, CMN_SUCCESS);
	return firstFreeBucketIndex;
}

template <typename T>
CmnHandle cmnInsert(CmnHandleMap<T>* map, const T& element, CmnResult* result) {
	CmnResult localResult;

	uint32_t freeBucketIndex = cmnHandleMapGetNextFreeBucket(map, &localResult);
	if (localResult != CMN_SUCCESS) {
		CMN_SET_RESULT(result, localResult);
		return {};
	}

	CmnHandleMapBucket<T>* bucket = &map->buckets[freeBucketIndex];
	bucket->generation++;
	bucket->isInUse = true;
	bucket->element = element;

	return CmnHandle{
		/*index=*/	freeBucketIndex,
		/*generation=*/	bucket->generation,
	};
}

template <typename T>
bool cmnIsValid(const CmnHandleMap<T>* map, CmnHandle handle) {
	return handle.index < map->buckets.length &&
		map->buckets[handle.index].isInUse &&
		handle.generation == map->buckets[handle.index].generation;
}

template <typename T>
T& cmnGet(CmnHandleMap<T>* map, CmnHandle handle, bool* wasHandleValid) {
	if (!cmnIsValid(map, handle)) {
		if (wasHandleValid != nullptr) {
			*wasHandleValid = false;
		}

		return map->defaultElement;
	} else {
		if (wasHandleValid != nullptr) {
			*wasHandleValid = true;
		}

		return map->buckets[handle.index].element;
	}
}

template <typename T>
void cmnRemove(CmnHandleMap<T>* map, CmnHandle handle) {
	if (!cmnIsValid(map, handle)) {
		return;
	}

	CmnHandleMapBucket<T>* bucket = &map->buckets[handle.index];
	bucket->isInUse = false;
	bucket->nextFreeIndex = map->firstFree;

	map->firstFree = handle.index;
}

#endif // CMN_HANDLEMAP_H

