#include "test.h"

#include <stdlib.h>
#include <lib/common/arena.h>
#include <lib/common/handle_map.h>

void checkForHandleMapDataCoherency(Test* test) {
	CmnResult result;

	uint8_t* memory = (uint8_t*)malloc(1024 * sizeof(int32_t));
	if (memory == nullptr) {
		testOutOfMemory(test);
	}

	CmnArena arena = cmnCreateArena(memory, 1024 * sizeof(int32_t), true);
	CmnAllocator arenaAllocator = cmnArenaAllocator(&arena);

	CmnHandleMap<int32_t> map;
	cmnCreateHandleMap(&map, arenaAllocator, 0, &result);
	TEST_ASSERT(test, result == CMN_SUCCESS);

	CmnHandle handles[128];
	for (int32_t i = 0; i < 128; i++) {
		handles[i] = cmnInsert(&map, i, &result);
		TEST_ASSERT(test, result == CMN_SUCCESS);
	}

	for (int32_t i = 0; i < 128; i++) {
		bool wasHandleValid;
		int32_t element = cmnGet(&map, handles[i], &wasHandleValid);

		TEST_ASSERT(test, wasHandleValid);
		TEST_ASSERT(test, element == i);
	}
}

void checkForHandleMapBucketReusage(Test* test) {
	CmnResult result;

	uint8_t* memory = (uint8_t*)malloc(1024 * sizeof(int32_t));
	if (memory == nullptr) {
		testOutOfMemory(test);
	}

	CmnArena arena = cmnCreateArena(memory, 1024 * sizeof(int32_t), true);
	CmnAllocator arenaAllocator = cmnArenaAllocator(&arena);

	CmnHandleMap<int32_t> map;
	cmnCreateHandleMap(&map, arenaAllocator, 0, &result);
	TEST_ASSERT(test, result == CMN_SUCCESS);

	CmnHandle first = cmnInsert(&map, 0, &result);
	TEST_ASSERT(test, result == CMN_SUCCESS);

	cmnRemove(&map, first);

	CmnHandle second = cmnInsert(&map, 1, &result);
	TEST_ASSERT(test, result == CMN_SUCCESS);

	TEST_ASSERT(test, first.index == second.index);
	TEST_ASSERT(test, (first.generation + 1) == second.generation);
}

void checkForHandleMapGenerationOverflowBehaviour(Test* test) {
	CmnResult result;

	uint8_t* memory = (uint8_t*)malloc(1024 * sizeof(int32_t));
	if (memory == nullptr) {
		testOutOfMemory(test);
	}

	CmnArena arena = cmnCreateArena(memory, 1024 * sizeof(int32_t), true);
	CmnAllocator arenaAllocator = cmnArenaAllocator(&arena);

	CmnHandleMap<int32_t> map;
	cmnCreateHandleMap(&map, arenaAllocator, 0, &result);
	TEST_ASSERT(test, result == CMN_SUCCESS);

	// Simulate a lot of allocations and deallocations
	map.buckets[0].generation = UINT32_MAX;

	CmnHandle handle = cmnInsert(&map, 0, &result);
	TEST_ASSERT(test, handle.index == 1);
}

void checkForHandleMapIndexOverflowBehaviour(Test* test) {
	CmnResult result;

	uint8_t* memory = (uint8_t*)malloc(1024 * sizeof(int32_t));
	if (memory == nullptr) {
		testOutOfMemory(test);
	}

	CmnArena arena = cmnCreateArena(memory, 1024 * sizeof(int32_t), true);
	CmnAllocator arenaAllocator = cmnArenaAllocator(&arena);

	CmnHandleMap<int32_t> map;
	cmnCreateHandleMap(&map, arenaAllocator, 0, &result);
	TEST_ASSERT(test, result == CMN_SUCCESS);

	// Simulate a lot of allocations
	map.firstFree = UINT32_MAX;

	CmnHandle handle = cmnInsert(&map, 0, &result);
	TEST_ASSERT(test, handle.index == 0 && handle.generation == 0);
	TEST_ASSERT(test, result == CMN_OUT_OF_RESOURCE_SLOTS);
}

void checkForHandleMapInvalidHandleBehaviour(Test* test) {
	CmnResult result;

	uint8_t* memory = (uint8_t*)malloc(1024 * sizeof(int32_t));
	if (memory == nullptr) {
		testOutOfMemory(test);
	}

	CmnArena arena = cmnCreateArena(memory, 1024 * sizeof(int32_t), true);
	CmnAllocator arenaAllocator = cmnArenaAllocator(&arena);

	CmnHandleMap<int32_t> map;
	cmnCreateHandleMap(&map, arenaAllocator, 42, &result);
	TEST_ASSERT(test, result == CMN_SUCCESS);

	bool wasHandleValid;
	int32_t element = cmnGet(&map, { 10, 20 }, &wasHandleValid);

	TEST_ASSERT(test, !wasHandleValid);
	TEST_ASSERT(test, element == 42);
}

