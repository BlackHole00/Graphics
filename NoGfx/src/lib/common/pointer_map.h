#ifndef CMN_POINTERMAP_H
#define CMN_POINTERMAP_H

#include <lib/common/exponential_array.h>

#define CMN_POINTERMAP_FREE ((uintptr_t)0)
#define CMN_POINTERMAP_DELETED ((uintptr_t)(~(uintptr_t)0))

template <typename T>
struct CmnPointerMapBucket {
	uintptr_t	key;
	T		value;
};

template <typename T>
struct CmnPointerMap {
	CmnAllocator		allocator;

	CmnPointerMapBucket<T>*	buckets;
	size_t			length;
	size_t			capacity;
	T			defaultValue;
};

inline size_t cmnHashPointer(uintptr_t ptr) {
	uint64_t hash = ptr + 0x9e3779b97f4a7c15ULL;
	hash = (hash ^ (hash >> 30)) * 0xbf58476d1ce4e5b9ULL;
	hash = (hash ^ (hash >> 27)) * 0x94d049bb133111ebULL;
	hash = hash ^ (hash >> 31);

	return (size_t)hash;
}

template <typename T>
void cmnCreatePointerMap(CmnPointerMap<T>* map, size_t initialCapacity, T defaultValue, CmnAllocator allocator, CmnResult* result) {
	CmnResult localResult;

	if (initialCapacity == 0) {
		initialCapacity = 256;
	}
	
	CmnPointerMapBucket<T>* buckets = cmnAlloc<CmnPointerMapBucket<T>>(allocator, initialCapacity, &localResult);
	if (localResult != CMN_SUCCESS) {
		CMN_SET_RESULT(result, localResult);
		return;
	}
	
	map->allocator		= allocator;
	map->buckets		= buckets;
	map->length		= 0;
	map->defaultValue	= defaultValue;
	map->capacity		= initialCapacity;
	CMN_SET_RESULT(result, CMN_SUCCESS);
}

template <typename T>
void cmnDestroyPointerMap(CmnPointerMap<T>* map) {
	if (map->allocator.vtable != nullptr) {
		cmnFree(map->allocator, map->buckets);
	}

	*map = {};
}

template <typename T>
void cmnReserve(CmnPointerMap<T>* map, size_t capacity, CmnResult* result) {
	if (capacity < map->length) {
		CMN_SET_RESULT(result, CMN_INVALID_PARAMETERS);
		return;
	}

	CmnResult localResult;
	CmnPointerMapBucket<T>* oldBuckets = map->buckets;
	size_t oldCapacity = map->capacity;

	CmnPointerMapBucket<T>* newBuckets = cmnAlloc<CmnPointerMapBucket<T>>(map->allocator, capacity, &localResult);
	if (localResult != CMN_SUCCESS) {
		CMN_SET_RESULT(result, localResult);
		return;
	}

	map->buckets	= newBuckets;
	map->length	= 0;
	map->capacity	= capacity;

	for (size_t i = 0; i < oldCapacity; i++) {
		CmnPointerMapBucket<T>* bucket = &oldBuckets[i];
		if (bucket->key == CMN_POINTERMAP_FREE || bucket->key == CMN_POINTERMAP_DELETED) {
			continue;
		}

		cmnInsert(map, bucket->key, bucket->value, nullptr);
	}

	cmnFree(map->allocator, oldBuckets);

	CMN_SET_RESULT(result, CMN_SUCCESS);
}

template <typename T>
void cmnInsert(CmnPointerMap<T>* map, uintptr_t key, const T& value, CmnResult* result) {
	if (key == 0) {
		CMN_SET_RESULT(result, CMN_INVALID_PARAMETERS);
		return;
	}

	if (map->length == map->capacity) {
		CmnResult localResult;
		cmnReserve(map, map->capacity * 2, &localResult);

		if (localResult != CMN_SUCCESS) {
			CMN_SET_RESULT(result, localResult);
			return;
		}
	}

	size_t hash = cmnHashPointer(key) % map->capacity;
	for (;;) {
		if (map->buckets[hash].key == CMN_POINTERMAP_FREE || map->buckets[hash].key == CMN_POINTERMAP_DELETED) {
			break;
		}

		hash = hash + 1;
		hash %= map->capacity;
	}

	map->buckets[hash].key = key;
	map->buckets[hash].value = value;

	map->length++;
}

template <typename T>
T& cmnGet(CmnPointerMap<T>* map, uintptr_t key, bool* didFind) {
	size_t hash = cmnHashPointer(key) % map->capacity;
	for (;;) {
		if (map->buckets[hash].key == CMN_POINTERMAP_FREE) {
			if (didFind != nullptr) {
				*didFind = false;
			}
			return map->defaultValue;
		}

		if (map->buckets[hash].key == key) {
			if (didFind != nullptr) {
				*didFind = true;
			}
			return map->buckets[hash].value;
		}

		hash = hash + 1;
		hash %= map->capacity;
	}
}

template <typename T>
bool cmnContains(CmnPointerMap<T>* map, uintptr_t key) {
	size_t hash = cmnHashPointer(key) % map->capacity;
	for (;;) {
		if (map->buckets[hash].key == CMN_POINTERMAP_FREE) {
			return false;
		}

		if (map->buckets[hash].key == key) {
			return true;
		}

		hash = hash + 1;
		hash %= map->capacity;
	}
}

template <typename T>
void cmnRemove(CmnPointerMap<T>* map, uintptr_t key) {
	size_t hash = cmnHashPointer(key) % map->capacity;
	for (;;) {
		if (map->buckets[hash].key == CMN_POINTERMAP_FREE) {
			return;
		}

		if (map->buckets[hash].key == key) {
			map->buckets[hash].key = CMN_POINTERMAP_DELETED;
			map->buckets[hash].value = {};

			return;
		}

		hash = hash + 1;
		hash %= map->capacity;
	}
}


#endif // CMN_POINTERMAP_H

