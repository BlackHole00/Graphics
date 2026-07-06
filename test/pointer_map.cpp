#include "test.h"

#include <stdlib.h>
#include <lib/common/common.h>
#include <lib/common/pointer_map.h>
#include <lib/common/heap_allocator.h>

void checkPointerMapCreation(Test* test) {
	CmnResult result;

	CmnAllocator allocator = cmnHeapAllocator();

	CmnPointerMap<int32_t> map;
	cmnCreatePointerMap(&map, 16, -1, allocator, &result);
	TEST_ASSERT(test, result == CMN_SUCCESS);
	TEST_ASSERT(test, map.capacity >= 16);
	TEST_ASSERT(test, map.length == 0);

	cmnDestroyPointerMap(&map);
}

void checkPointerMapInsertAndContains(Test* test) {
	CmnAllocator allocator = cmnHeapAllocator();

	CmnPointerMap<int32_t> map;
	CmnResult result;
	cmnCreatePointerMap(&map, 8, -1, allocator, &result);
	TEST_ASSERT(test, result == CMN_SUCCESS);

	cmnInsert(&map, 1, 100, nullptr);
	cmnInsert(&map, 2, 200, nullptr);
	cmnInsert(&map, 3, 300, nullptr);

	TEST_ASSERT(test, cmnContains(&map, 1));
	TEST_ASSERT(test, cmnContains(&map, 2));
	TEST_ASSERT(test, cmnContains(&map, 3));
	TEST_ASSERT(test, !cmnContains(&map, 4));
}

void checkPointerMapGet(Test* test) {
	CmnAllocator allocator = cmnHeapAllocator();

	CmnPointerMap<int32_t> map;
	CmnResult result;
	cmnCreatePointerMap(&map, 8, -42, allocator, &result);
	TEST_ASSERT(test, result == CMN_SUCCESS);

	cmnInsert(&map, 10, 500, nullptr);

	bool found = false;
	int32_t value = cmnGet(&map, 10, &found);
	TEST_ASSERT(test, found && value == 500);

	value = cmnGet(&map, 11, &found);
	TEST_ASSERT(test, !found && value == map.defaultValue);
}

void checkPointerMapRemove(Test* test) {
	CmnAllocator allocator = cmnHeapAllocator();

	CmnPointerMap<int32_t> map;
	CmnResult result;
	cmnCreatePointerMap(&map, 8, -1, allocator, &result);
	TEST_ASSERT(test, result == CMN_SUCCESS);

	cmnInsert(&map, 1, 100, nullptr);
	cmnInsert(&map, 2, 200, nullptr);
	cmnInsert(&map, 3, 300, nullptr);

	cmnRemove(&map, 2);
	TEST_ASSERT(test, !cmnContains(&map, 2));
	TEST_ASSERT(test, cmnContains(&map, 1));
	TEST_ASSERT(test, cmnContains(&map, 3));
}

void checkPointerMapReserveAndRehash(Test* test) {
	CmnAllocator allocator = cmnHeapAllocator();

	CmnPointerMap<int32_t> map;
	CmnResult result;
	cmnCreatePointerMap(&map, 4, -1, allocator, &result);
	TEST_ASSERT(test, result == CMN_SUCCESS);

	for (int i = 1; i <= 50; i++) {
	cmnInsert(&map, (uintptr_t)i, i * 10, nullptr);
	}

	for (int i = 1; i <= 50; i++) {
	TEST_ASSERT(test, cmnContains(&map, (uintptr_t)i));
	}

	TEST_ASSERT(test, map.capacity > 4);
}
