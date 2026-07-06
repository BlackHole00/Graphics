#include "test.h"

#include <stdlib.h>

#include <lib/common/arena.h>
#include <lib/common/chain.h>

static void createChainFixture(CmnArena* arena, CmnAllocator* allocator, CmnChain<int32_t, 4>* chain, uint8_t** memory, CmnResult* result) {
	*memory = (uint8_t*)malloc(256 * sizeof(int32_t));
	if (*memory == nullptr) {
		testOutOfMemory(nullptr);
	}

	*arena = cmnCreateArena(*memory, 256 * sizeof(int32_t), true);
	*allocator = cmnArenaAllocator(arena);

	cmnCreateChain(chain);

        CMN_SET_RESULT(result, CMN_SUCCESS);
}

void checkChainCreationAndInsertion(Test* test) {
	CmnResult result;
	CmnArena arena;
	CmnAllocator allocator;
	CmnChain<int32_t, 4> chain;
	uint8_t* memory = nullptr;

	createChainFixture(&arena, &allocator, &chain, &memory, &result);
	TEST_ASSERT(test, result == CMN_SUCCESS);

	for (int32_t i = 0; i < 11; i++) {
		cmnInsert(&chain, i, allocator, &result);
		TEST_ASSERT(test, result == CMN_SUCCESS);
	}

	cmnDestroyChain(&chain, allocator);
	free(memory);
}

void checkChainContainsAndRemove(Test* test) {
	CmnResult result;
	CmnArena arena;
	CmnAllocator allocator;
	CmnChain<int32_t, 4> chain;
	uint8_t* memory = nullptr;

	createChainFixture(&arena, &allocator, &chain, &memory, &result);
	TEST_ASSERT(test, result == CMN_SUCCESS);

	for (int32_t i = 0; i < 11; i++) {
		cmnInsert(&chain, i, allocator, &result);
		TEST_ASSERT(test, result == CMN_SUCCESS);
	}

	TEST_ASSERT(test, cmnContains(&chain, 0));
	TEST_ASSERT(test, cmnContains(&chain, 5));
	TEST_ASSERT(test, cmnContains(&chain, 10));
	TEST_ASSERT(test, !cmnContains(&chain, 11));

	cmnRemove(&chain, 0, allocator);
	cmnRemove(&chain, 5, allocator);
	cmnRemove(&chain, 10, allocator);

	TEST_ASSERT(test, !cmnContains(&chain, 0));
	TEST_ASSERT(test, !cmnContains(&chain, 5));
	TEST_ASSERT(test, !cmnContains(&chain, 10));

	cmnDestroyChain(&chain, allocator);
	free(memory);
}

void checkChainIteration(Test* test) {
	CmnResult result;
	CmnArena arena;
	CmnAllocator allocator;
	CmnChain<int32_t, 4> chain;
	uint8_t* memory = nullptr;

	createChainFixture(&arena, &allocator, &chain, &memory, &result);
	TEST_ASSERT(test, result == CMN_SUCCESS);

	for (int32_t i = 0; i < 11; i++) {
		cmnInsert(&chain, i, allocator, &result);
		TEST_ASSERT(test, result == CMN_SUCCESS);
	}

	cmnRemove(&chain, 0, allocator);
	cmnRemove(&chain, 5, allocator);
	cmnRemove(&chain, 10, allocator);

	bool seen[11] = {};
	CmnChainIterator<int32_t, 4> iter;
	cmnCreateChainIterator(&chain, &iter);

	int32_t* value = nullptr;
	size_t count = 0;
	while (cmnIterate(&iter, &value)) {
		TEST_ASSERT(test, *value >= 0 && *value < 11);
		TEST_ASSERT(test, !seen[*value]);
		seen[*value] = true;
		count++;
	}

	TEST_ASSERT(test, count == 8);
	TEST_ASSERT(test, seen[1]);
	TEST_ASSERT(test, seen[2]);
	TEST_ASSERT(test, seen[3]);
	TEST_ASSERT(test, seen[4]);
	TEST_ASSERT(test, !seen[0]);
	TEST_ASSERT(test, !seen[5]);
	TEST_ASSERT(test, !seen[10]);

	cmnDestroyChain(&chain, allocator);
	free(memory);
}