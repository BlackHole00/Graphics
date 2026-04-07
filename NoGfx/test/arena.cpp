#include "test.h"

#include <stdlib.h>
#include <lib/common/arena.h>

void checkForArenaMemoryCoherency(Test* test) {
	size_t backingMemorySize = 1024 * sizeof(uint32_t);
	uint8_t* backingMemory = (uint8_t*)malloc(backingMemorySize);
	if (backingMemory == nullptr) {
		testOutOfMemory(test);
	}

	CmnArena arena = cmnCreateArena(backingMemory, backingMemorySize);

	for (size_t i = 0; i < 32; i++) {
		uint32_t* memory = cmnArenaAlloc<uint32_t>(&arena, 32, nullptr);
		for (size_t j = 0; j < 32; j++) {
			memory[j] = i * 32 + j;
		}
	}

	for (size_t i = 0; i < 1024; i++) {
		uint32_t* memory = (uint32_t*)arena.backing;
		TEST_ASSERT(test, memory[i] == (uint32_t)i);
	}
}

