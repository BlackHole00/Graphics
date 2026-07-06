#include "test.h"

#include <stdlib.h>
#include <lib/common/common.h>
#include <lib/common/pool.h>

void checkForPoolInitialMemorySetup(Test* test) {
	uint8_t* backingMemory = (uint8_t*)malloc(16 * sizeof(int32_t));
	if (backingMemory == nullptr) {
		testOutOfMemory(test);
	}

	cmnCreatePool(backingMemory, 16 * sizeof(int32_t), sizeof(int32_t), 16);

	for (size_t i = 0; i < 16; i++) {
		size_t offset = i * sizeof(CmnPoolBlockHeader);
		CmnPoolBlockHeader* header = (CmnPoolBlockHeader*)&backingMemory[offset];

		TEST_ASSERT(test, header->initialized);
		TEST_ASSERT(test, header->nextFreeBlock == i+1);
	}
}

void checkPoolBlockReusage(Test* test) {
	uint8_t* backingMemory = (uint8_t*)malloc(16 * sizeof(int32_t));
	if (backingMemory == nullptr) {
		testOutOfMemory(test);
	}

	CmnPool pool = cmnCreatePool(backingMemory, 16 * sizeof(int32_t), sizeof(int32_t), 16);

	int32_t* firstAllocation = cmnPoolAlloc<int32_t>(&pool, nullptr);
	cmnPoolAlloc<int32_t>(&pool, nullptr);
	cmnPoolAlloc<int32_t>(&pool, nullptr);
	cmnPoolAlloc<int32_t>(&pool, nullptr);

	cmnPoolFree(&pool, firstAllocation);
	int32_t* secondAllocation = cmnPoolAlloc<int32_t>(&pool, nullptr);

	TEST_ASSERT(test, firstAllocation == secondAllocation);
}

void checkPoolOOMBehaviour(Test* test) {
	uint8_t* backingMemory = (uint8_t*)malloc(16 * sizeof(int32_t));
	if (backingMemory == nullptr) {
		testOutOfMemory(test);
	}

	CmnPool pool = cmnCreatePool(backingMemory, 16 * sizeof(int32_t), sizeof(int32_t), 16);

	for (int i = 0; i < 16; i++) {
		cmnPoolAlloc<int32_t>(&pool, nullptr);
	}

	CmnResult result;
	int32_t* oomAllocation = cmnPoolAlloc<int32_t>(&pool, &result);

	TEST_ASSERT(test, oomAllocation == nullptr);
	TEST_ASSERT(test, result == CMN_OUT_OF_MEMORY);
}

void checkPoolUninitializedLocations(Test* test) {
	uint8_t* backingMemory = (uint8_t*)malloc(16 * sizeof(int32_t));
	if (backingMemory == nullptr) {
		testOutOfMemory(test);
	}

	CmnPool pool = cmnCreatePool(backingMemory, 16 * sizeof(int32_t), sizeof(int32_t), 0);

	for (int i = 0; i < 16; i++) {
		int32_t* allocation = cmnPoolAlloc<int32_t>(&pool, nullptr);
		*allocation = i;
	}

	int32_t* memoryInt32 = (int32_t*)backingMemory;
	for (int i = 0; i < 16; i++) {
		TEST_ASSERT(test, memoryInt32[i] == i);
	}
}

