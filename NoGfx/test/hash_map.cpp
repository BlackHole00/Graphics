#include "test.h"

#include <stdlib.h>
#include <lib/common/common.h>
#include <lib/common/hash_map.h>
#include <lib/common/heap_allocator.h>

typedef struct CmnTestPointKey {
	int32_t x;
	int32_t y;
} CmnTestPointKey;

template <>
struct CmnTypeTraits<CmnTestPointKey> {
	static bool eq(const CmnTestPointKey& left, const CmnTestPointKey& right) {
		return left.x == right.x && left.y == right.y;
	}

	static CmnCmp cmp(const CmnTestPointKey& left, const CmnTestPointKey& right) {
		if (left.x == right.x && left.y == right.y) {
			return CMN_EQUALS;
		}
		if (left.x < right.x || (left.x == right.x && left.y < right.y)) {
			return CMN_LESS;
		}
		return CMN_MORE;
	}

	static size_t hash(const CmnTestPointKey& value) {
		uint64_t packed = ((uint64_t)(uint32_t)value.x << 32) | (uint32_t)value.y;
		return cmnHashInteger64(packed);
	}
};

void checkHashMapCreation(Test* test) {
	CmnResult result;
	CmnAllocator allocator = cmnHeapAllocator();

	CmnHashMap<int32_t, int32_t> map;
	cmnCreateHashMap(&map, 16, -1, allocator, &result);
	TEST_ASSERT(test, result == CMN_SUCCESS);
	TEST_ASSERT(test, map.capacity >= 16);
	TEST_ASSERT(test, map.length == 0);

	cmnDestroyHashMap(&map);
}

void checkHashMapInsertContainsAndGet(Test* test) {
	CmnResult result;
	CmnAllocator allocator = cmnHeapAllocator();

	CmnHashMap<int32_t, int32_t> map;
	cmnCreateHashMap(&map, 8, -42, allocator, &result);
	TEST_ASSERT(test, result == CMN_SUCCESS);

	cmnInsert(&map, 10, 100, nullptr);
	cmnInsert(&map, 20, 200, nullptr);
	cmnInsert(&map, 30, 300, nullptr);

	TEST_ASSERT(test, cmnContains(&map, 10));
	TEST_ASSERT(test, cmnContains(&map, 20));
	TEST_ASSERT(test, cmnContains(&map, 30));
	TEST_ASSERT(test, !cmnContains(&map, 40));

	bool found = false;
	int32_t value = cmnGet(&map, 20, &found);
	TEST_ASSERT(test, found && value == 200);

	value = cmnGet(&map, 99, &found);
	TEST_ASSERT(test, !found && value == map.defaultValue);

	cmnDestroyHashMap(&map);
}

void checkHashMapOverwriteDoesNotGrowLength(Test* test) {
	CmnResult result;
	CmnAllocator allocator = cmnHeapAllocator();

	CmnHashMap<int32_t, int32_t> map;
	cmnCreateHashMap(&map, 8, -1, allocator, &result);
	TEST_ASSERT(test, result == CMN_SUCCESS);

	cmnInsert(&map, 7, 70, nullptr);
	size_t lengthAfterFirstInsert = map.length;

	cmnInsert(&map, 7, 700, nullptr);
	TEST_ASSERT(test, map.length == lengthAfterFirstInsert);

	bool found = false;
	int32_t value = cmnGet(&map, 7, &found);
	TEST_ASSERT(test, found && value == 700);

	cmnDestroyHashMap(&map);
}

void checkHashMapRemoveAndReuseDeletedSlots(Test* test) {
	CmnResult result;
	CmnAllocator allocator = cmnHeapAllocator();

	CmnHashMap<CmnTestPointKey, int32_t> map;
	cmnCreateHashMap(&map, 4, -1, allocator, &result);
	TEST_ASSERT(test, result == CMN_SUCCESS);

	CmnTestPointKey k1 = { 1, 2 };
	CmnTestPointKey k2 = { 3, 4 };
	CmnTestPointKey k3 = { 5, 6 };

	cmnInsert(&map, k1, 12, nullptr);
	cmnInsert(&map, k2, 34, nullptr);

	cmnRemove(&map, k1);
	TEST_ASSERT(test, !cmnContains(&map, k1));
	TEST_ASSERT(test, cmnContains(&map, k2));

	cmnInsert(&map, k3, 56, nullptr);
	TEST_ASSERT(test, cmnContains(&map, k3));

	bool found = false;
	int32_t value = cmnGet(&map, k3, &found);
	TEST_ASSERT(test, found && value == 56);

	cmnDestroyHashMap(&map);
}

void checkHashMapReserveAndRehash(Test* test) {
	CmnResult result;
	CmnAllocator allocator = cmnHeapAllocator();

	CmnHashMap<int32_t, int32_t> map;
	cmnCreateHashMap(&map, 4, -1, allocator, &result);
	TEST_ASSERT(test, result == CMN_SUCCESS);

	for (int i = 1; i <= 128; i++) {
		cmnInsert(&map, i, i * 10, nullptr);
	}

	for (int i = 1; i <= 128; i++) {
		bool found = false;
		int32_t value = cmnGet(&map, i, &found);
		TEST_ASSERT(test, found);
		TEST_ASSERT(test, value == i * 10);
	}

	TEST_ASSERT(test, map.capacity > 4);
	cmnDestroyHashMap(&map);
}
