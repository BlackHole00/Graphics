#ifndef CMN_HASHMAP_H
#define CMN_HASHMAP_H

#include <lib/common/common.h>
#include <lib/common/allocator.h>
#include <lib/common/type_traits.h>

// Bucket state meaning the bucket was never used.
#define CMN_HASHMAP_FREE 0
// Bucket state meaning the bucket currently stores a key-value pair.
#define CMN_HASHMAP_OCCUPIED 1
// Bucket state meaning the bucket used to store a key-value pair but is now deleted.
#define CMN_HASHMAP_DELETED 2

// Bucket used internally by CmnHashMap.
template <typename K, typename V>
struct CmnHashMapBucket {
	uint8_t	state;
	K	key;
	V	value;
};

// Open-addressing hash map using CmnTypeTraits<K>::eq/hash.
template <typename K, typename V>
struct CmnHashMap {
	CmnAllocator		allocator;

	CmnHashMapBucket<K, V>*	buckets;
	size_t			length;
	size_t			capacity;
	V			defaultValue;
};

// Initializes a hash map.
//
// Inputs:
// - map: Map to initialize.
// - initialCapacity: Initial bucket capacity, or 256 when zero.
// - defaultValue: Value returned by failed lookups.
// - allocator: Allocator used for bucket storage.
// - result: Optional operation result.
//
// Results:
// - CMN_SUCCESS: Initialization succeeded.
// - CMN_OUT_OF_MEMORY: Backing allocator ran out of memory.
template <typename K, typename V>
void cmnCreateHashMap(CmnHashMap<K, V>* map, size_t initialCapacity, const V& defaultValue, CmnAllocator allocator, CmnResult* result) {
	CmnResult localResult;

	if (initialCapacity == 0) {
		initialCapacity = 256;
	}

	CmnHashMapBucket<K, V>* buckets = cmnAlloc<CmnHashMapBucket<K, V>>(allocator, initialCapacity, &localResult);
	if (localResult != CMN_SUCCESS) {
		CMN_SET_RESULT(result, localResult);
		return;
	}

	map->allocator = allocator;
	map->buckets = buckets;
	map->length = 0;
	map->defaultValue = defaultValue;
	map->capacity = initialCapacity;
	CMN_SET_RESULT(result, CMN_SUCCESS);
}

// Destroys a hash map and releases storage.
//
// Inputs:
// - map: Map to destroy.
template <typename K, typename V>
void cmnDestroyHashMap(CmnHashMap<K, V>* map) {
	if (map->allocator.vtable != nullptr) {
		cmnFree(map->allocator, map->buckets);
	}

	*map = {};
}

// Finds an insertion position for key.
//
// Returns:
// - Index where key should be inserted or overwritten.
// - map->capacity when no slot is available.
template <typename K, typename V>
size_t cmnHashMapFindInsertIndex(CmnHashMap<K, V>* map, const K& key) {
	size_t hash = cmnHash(key) % map->capacity;
	size_t firstDeleted = map->capacity;

	for (size_t probe = 0; probe < map->capacity; probe++) {
		CmnHashMapBucket<K, V>* bucket = &map->buckets[hash];

		if (bucket->state == CMN_HASHMAP_FREE) {
			if (firstDeleted != map->capacity) {
				return firstDeleted;
			}
			return hash;
		}

		if (bucket->state == CMN_HASHMAP_DELETED) {
			if (firstDeleted == map->capacity) {
				firstDeleted = hash;
			}
		} else if (cmnEq(bucket->key, key)) {
			return hash;
		}

		hash = (hash + 1) % map->capacity;
	}

	return firstDeleted;
}

// Ensures at least capacity buckets.
//
// Inputs:
// - map: Target map.
// - capacity: Requested bucket capacity.
// - result: Optional operation result.
//
// Results:
// - CMN_SUCCESS: Reserve succeeded.
// - CMN_OUT_OF_MEMORY: Backing allocator ran out of memory.
// - CMN_INVALID_PARAMETERS: Requested capacity is smaller than current length.
template <typename K, typename V>
void cmnReserve(CmnHashMap<K, V>* map, size_t capacity, CmnResult* result) {
	if (capacity < map->length || capacity == 0) {
		CMN_SET_RESULT(result, CMN_INVALID_PARAMETERS);
		return;
	}

	CmnResult localResult;
	CmnHashMapBucket<K, V>* oldBuckets = map->buckets;
	size_t oldCapacity = map->capacity;

	CmnHashMapBucket<K, V>* newBuckets = cmnAlloc<CmnHashMapBucket<K, V>>(map->allocator, capacity, &localResult);
	if (localResult != CMN_SUCCESS) {
		CMN_SET_RESULT(result, localResult);
		return;
	}

	map->buckets = newBuckets;
	map->length = 0;
	map->capacity = capacity;

	for (size_t i = 0; i < oldCapacity; i++) {
		CmnHashMapBucket<K, V>* bucket = &oldBuckets[i];
		if (bucket->state != CMN_HASHMAP_OCCUPIED) {
			continue;
		}

		cmnInsert(map, bucket->key, bucket->value, nullptr);
	}

	cmnFree(map->allocator, oldBuckets);

	CMN_SET_RESULT(result, CMN_SUCCESS);
}

// Inserts or overwrites a key-value pair.
//
// Inputs:
// - map: Target map.
// - key: Key to insert.
// - value: Value to associate with key.
// - result: Optional operation result.
//
// Results:
// - CMN_SUCCESS: Insert succeeded.
// - CMN_OUT_OF_MEMORY: Backing allocator ran out of memory during growth.
template <typename K, typename V>
void cmnInsert(CmnHashMap<K, V>* map, const K& key, const V& value, CmnResult* result) {
	if ((map->length + 1) * 4 >= map->capacity * 3) {
		CmnResult localResult;
		cmnReserve(map, map->capacity * 2, &localResult);
		if (localResult != CMN_SUCCESS) {
			CMN_SET_RESULT(result, localResult);
			return;
		}
	}

	size_t index = cmnHashMapFindInsertIndex(map, key);
	if (index == map->capacity) {
		CMN_SET_RESULT(result, CMN_OUT_OF_MEMORY);
		return;
	}

	bool isNewElement = map->buckets[index].state != CMN_HASHMAP_OCCUPIED;
	map->buckets[index].state = CMN_HASHMAP_OCCUPIED;
	map->buckets[index].key = key;
	map->buckets[index].value = value;
	if (isNewElement) {
		map->length += 1;
	}

	CMN_SET_RESULT(result, CMN_SUCCESS);
}

// Returns the value associated with key.
//
// Inputs:
// - map: Target map.
// - key: Key to search.
// - didFind: Optional output flag.
//
// Returns:
// - Value reference when found, or defaultValue otherwise.
template <typename K, typename V>
V& cmnGet(CmnHashMap<K, V>* map, const K& key, bool* didFind) {
	size_t hash = cmnHash(key) % map->capacity;
	for (size_t probe = 0; probe < map->capacity; probe++) {
		CmnHashMapBucket<K, V>* bucket = &map->buckets[hash];

		if (bucket->state == CMN_HASHMAP_FREE) {
			if (didFind != nullptr) {
				*didFind = false;
			}
			return map->defaultValue;
		}

		if (bucket->state == CMN_HASHMAP_OCCUPIED && cmnEq(bucket->key, key)) {
			if (didFind != nullptr) {
				*didFind = true;
			}
			return bucket->value;
		}

		hash = (hash + 1) % map->capacity;
	}

	if (didFind != nullptr) {
		*didFind = false;
	}
	return map->defaultValue;
}

// Checks whether key exists in the map.
//
// Inputs:
// - map: Target map.
// - key: Key to search.
//
// Returns:
// - true when key exists.
template <typename K, typename V>
bool cmnContains(CmnHashMap<K, V>* map, const K& key) {
	bool didFind;
	cmnGet(map, key, &didFind);
	return didFind;
}

// Removes a key-value pair from the map.
//
// Inputs:
// - map: Target map.
// - key: Key to remove.
template <typename K, typename V>
void cmnRemove(CmnHashMap<K, V>* map, const K& key) {
	size_t hash = cmnHash(key) % map->capacity;
	for (size_t probe = 0; probe < map->capacity; probe++) {
		CmnHashMapBucket<K, V>* bucket = &map->buckets[hash];

		if (bucket->state == CMN_HASHMAP_FREE) {
			return;
		}

		if (bucket->state == CMN_HASHMAP_OCCUPIED && cmnEq(bucket->key, key)) {
			bucket->state = CMN_HASHMAP_DELETED;
			bucket->key = {};
			bucket->value = {};
			map->length -= 1;
			return;
		}

		hash = (hash + 1) % map->capacity;
	}
}

#endif // CMN_HASHMAP_H
