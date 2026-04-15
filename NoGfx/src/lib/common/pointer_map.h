#ifndef CMN_POINTERMAP_H
#define CMN_POINTERMAP_H

#include <lib/common/exponential_array.h>

/** Sentinel key marking a never-used bucket. */
#define CMN_POINTERMAP_FREE ((uintptr_t)0)
/** Sentinel key marking a deleted bucket. */
#define CMN_POINTERMAP_DELETED ((uintptr_t)(~(uintptr_t)0))

/**
	Bucket used internally by CmnPointerMap.
	@see CmnPointerMap
*/
template <typename T>
struct CmnPointerMapBucket {
	/** Key stored in this bucket. */
	uintptr_t	key;
	/** Value stored in this bucket. */
	T		value;
};

/**
	Open-addressing hash map keyed by pointer-sized integers.
*/
template <typename T>
struct CmnPointerMap {
	/** Allocator used for bucket storage. */
	CmnAllocator		allocator;

	/** Bucket array. */
	CmnPointerMapBucket<T>*	buckets;
	/** Number of elements currently stored. */
	size_t			length;
	/** Total number of buckets. */
	size_t			capacity;
	/** Value returned when lookup fails. */
	T			defaultValue;
};

/**
	Hashes a pointer-sized key.

	@param ptr The key to hash.

	@return The computed hash value.
*/
inline size_t cmnHashPointer(uintptr_t ptr) {
	uint64_t hash = ptr + 0x9e3779b97f4a7c15ULL;
	hash = (hash ^ (hash >> 30)) * 0xbf58476d1ce4e5b9ULL;
	hash = (hash ^ (hash >> 27)) * 0x94d049bb133111ebULL;
	hash = hash ^ (hash >> 31);

	return (size_t)hash;
}

/**
	Initializes a pointer map.

	@param map The map to initialize.
	@param initialCapacity Initial bucket capacity, or 256 when zero.
	@param defaultValue Value returned by failed lookups.
	@param allocator Allocator used for bucket storage.
	@param[out] result The result of the operation.
	@retval CMN_SUCCESS Pointer map initialization completed successfully.
	@retval CMN_OUT_OF_MEMORY Bucket allocation failed.
	@relates CmnPointerMap
*/
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

/**
	Destroys a pointer map and releases allocated storage.

	@param map The map to destroy.
	@relates CmnPointerMap
*/
template <typename T>
void cmnDestroyPointerMap(CmnPointerMap<T>* map) {
	if (map->allocator.vtable != nullptr) {
		cmnFree(map->allocator, map->buckets);
	}

	*map = {};
}

/**
	Reserves capacity for at least `capacity` elements.

	@param map The target map.
	@param capacity The requested bucket capacity.
	@param[out] result The result of the operation.
	@retval CMN_SUCCESS Capacity reservation completed successfully.
	@retval CMN_OUT_OF_MEMORY Bucket allocation failed.
	@relates CmnPointerMap
*/
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

/**
	Inserts or overwrites a key-value pair.

	@param map The target map.
	@param key The key to insert.
	@param value The value to associate with the key.
	@param[out] result The result of the operation.
	@retval CMN_SUCCESS The insertion completed successfully.
	@retval CMN_OUT_OF_MEMORY Internal growth failed while reserving additional capacity.
	@relates CmnPointerMap
*/
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

	CMN_SET_RESULT(result, CMN_SUCCESS);
	map->length++;
}

/**
	Gets the value associated with a key.

	@param map The target map.
	@param key The key to search.
	@param[out] didFind Optional output flag indicating whether the key was found.

	@return Reference to the stored value when found, or to `defaultValue` otherwise.
	@relates CmnPointerMap
*/
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

/**
	Checks whether a key exists in the map.

	@param map The target map.
	@param key The key to search.

	@return True if the key exists, false otherwise.
	@relates CmnPointerMap
*/
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

/**
	Removes a key-value pair from the map.

	@param map The target map.
	@param key The key to remove.
	@relates CmnPointerMap
*/
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

