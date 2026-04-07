#include "test.h"

#include <stdlib.h>
#include <lib/common/exponential_array.h>

void checkForExponentialArrayDataCoherency(Test* test) {
	uint8_t* memory = (uint8_t*)malloc(1024 * sizeof(int32_t));
	CmnArena arena = cmnCreateArena(memory, 1024 * sizeof(int32_t));

	CmnExponentialArray<int32_t> arr;
	cmnCreateExponentialArray(&arr, &arena);

	for (int32_t i = 0; i < 256; i++) {
		cmnAppend(&arr, i);
	}

	for (int32_t i = 0; i < 256; i++) {
		TEST_ASSERT(test, arr[i] == i);
	}
}

void checkForExponentialArrayMemoryCoherency(Test* test) {
	uint8_t* memory = (uint8_t*)malloc(1024 * sizeof(int32_t));
	CmnArena arena = cmnCreateArena(memory, 1024 * sizeof(int32_t));

	CmnExponentialArray<int32_t> arr;
	cmnCreateExponentialArray(&arr, &arena);

	for (int32_t i = 0; i < 256; i++) {
		cmnAppend(&arr, i);
	}

	int32_t* int32Memory = (int32_t*)memory;
	for (int32_t i = 0; i < 256; i++) {
		TEST_ASSERT(test, int32Memory[i] == i);
	}
}

