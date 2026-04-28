#include "test.h"

#include <stdlib.h>

#include <lib/common/arena.h>
#include <lib/common/keyed_chain.h>

typedef struct CmnTestKeyedChainPoint {
	int32_t x;
	int32_t y;
} CmnTestKeyedChainPoint;

template <>
struct CmnTypeTraits<CmnTestKeyedChainPoint> {
	static bool eq(const CmnTestKeyedChainPoint& left, const CmnTestKeyedChainPoint& right) {
		return left.x == right.x && left.y == right.y;
	}

	static CmnCmp cmp(const CmnTestKeyedChainPoint& left, const CmnTestKeyedChainPoint& right) {
		if (left.x == right.x && left.y == right.y) {
			return CMN_EQUALS;
		}
		if (left.x < right.x || (left.x == right.x && left.y < right.y)) {
			return CMN_LESS;
		}
		return CMN_MORE;
	}

	static size_t hash(const CmnTestKeyedChainPoint& value) {
		uint64_t packed = ((uint64_t)(uint32_t)value.x << 32) | (uint32_t)value.y;
		return cmnHashInteger64(packed);
	}
};

static void createKeyedChainFixture(
	Test* test,
	CmnArena* arena,
	CmnAllocator* allocator,
	CmnKeyedChain<CmnTestKeyedChainPoint, int32_t, 4>* chain,
	uint8_t** memory,
	CmnResult* result
) {
	*memory = (uint8_t*)malloc(256 * sizeof(int32_t));
	if (*memory == nullptr) {
		testOutOfMemory(test);
	}

	*arena = cmnCreateArena(*memory, 256 * sizeof(int32_t), true);
	*allocator = cmnArenaAllocator(arena);

	cmnCreateKeyedChain(chain, -1);

        CMN_SET_RESULT(result, CMN_SUCCESS);
}

void checkKeyedChainCreationAndInsertion(Test* test) {
	CmnResult result;
	CmnArena arena;
	CmnAllocator allocator;
	CmnKeyedChain<CmnTestKeyedChainPoint, int32_t, 4> chain;
	uint8_t* memory = nullptr;

	createKeyedChainFixture(test, &arena, &allocator, &chain, &memory, &result);
	TEST_ASSERT(test, result == CMN_SUCCESS);

	CmnTestKeyedChainPoint key0 = { 1, 2 };
	CmnTestKeyedChainPoint key1 = { 3, 4 };
	CmnTestKeyedChainPoint key2 = { 5, 6 };

	cmnInsert(&chain, key0, 10, allocator, &result);
	TEST_ASSERT(test, result == CMN_SUCCESS);
	cmnInsert(&chain, key1, 20, allocator, &result);
	TEST_ASSERT(test, result == CMN_SUCCESS);
	cmnInsert(&chain, key2, 30, allocator, &result);
	TEST_ASSERT(test, result == CMN_SUCCESS);

	TEST_ASSERT(test, cmnContains(&chain, key0));
	TEST_ASSERT(test, cmnContains(&chain, key1));
	TEST_ASSERT(test, cmnContains(&chain, key2));

	bool found = false;
	int32_t value = cmnGet(&chain, key1, &found);
	TEST_ASSERT(test, found && value == 20);

	cmnDestroyKeyedChain(&chain, allocator);
	free(memory);
}

void checkKeyedChainOverwriteAndRemoval(Test* test) {
	CmnResult result;
	CmnArena arena;
	CmnAllocator allocator;
	CmnKeyedChain<CmnTestKeyedChainPoint, int32_t, 4> chain;
	uint8_t* memory = nullptr;

	createKeyedChainFixture(test, &arena, &allocator, &chain, &memory, &result);
	TEST_ASSERT(test, result == CMN_SUCCESS);

	CmnTestKeyedChainPoint key = { 7, 8 };

	cmnInsert(&chain, key, 70, allocator, &result);
	TEST_ASSERT(test, result == CMN_SUCCESS);
	cmnInsert(&chain, key, 700, allocator, &result);
	TEST_ASSERT(test, result == CMN_SUCCESS);

	bool found = false;
	int32_t value = cmnGet(&chain, key, &found);
	TEST_ASSERT(test, found && value == 700);

	cmnRemove(&chain, key, allocator);
	TEST_ASSERT(test, !cmnContains(&chain, key));

	value = cmnGet(&chain, key, &found);
	TEST_ASSERT(test, !found && value == -1);

	cmnDestroyKeyedChain(&chain, allocator);
	free(memory);
}

void checkKeyedChainIteration(Test* test) {
	CmnResult result;
	CmnArena arena;
	CmnAllocator allocator;
	CmnKeyedChain<CmnTestKeyedChainPoint, int32_t, 4> chain;
	uint8_t* memory = nullptr;

	createKeyedChainFixture(test, &arena, &allocator, &chain, &memory, &result);
	TEST_ASSERT(test, result == CMN_SUCCESS);

	for (int32_t i = 0; i < 11; i++) {
		CmnTestKeyedChainPoint key = { i, i + 100 };
		cmnInsert(&chain, key, i * 10, allocator, &result);
		TEST_ASSERT(test, result == CMN_SUCCESS);
	}

	bool seen[11] = {};
	CmnKeyedChainIterator<CmnTestKeyedChainPoint, int32_t, 4> iter;
	cmnCreateKeyedChainIterator(&chain, &iter);

	CmnTestKeyedChainPoint* key = nullptr;
	int32_t* value = nullptr;
	size_t count = 0;
	while (cmnIterate(&iter, &key, &value)) {
		TEST_ASSERT(test, key->y == key->x + 100);
		TEST_ASSERT(test, *value == key->x * 10);
		TEST_ASSERT(test, !seen[key->x]);
		seen[key->x] = true;
		count++;
	}

	TEST_ASSERT(test, count == 11);
	for (int32_t i = 0; i < 11; i++) {
		TEST_ASSERT(test, seen[i]);
	}

	cmnDestroyKeyedChain(&chain, allocator);
	free(memory);
}