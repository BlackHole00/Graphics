#include "test.h"

#include <stdlib.h>
#include <lib/common/exponential_array.h>

void checkForExponentialArrayDataCoherency(Test* test) {
	CmnResult result;

	uint8_t* memory = (uint8_t*)malloc(1024 * sizeof(int32_t));
	if (memory == nullptr) {
		testOutOfMemory(test);
	}

	CmnArena arena = cmnCreateArena(memory, 1024 * sizeof(int32_t));

	CmnExponentialArray<int32_t> arr;
	cmnCreateExponentialArray(&arr, &arena, &result);
	TEST_ASSERT(test, result == CMN_SUCCESS);

	for (int32_t i = 0; i < 256; i++) {
		cmnAppend(&arr, i, &result);
		TEST_ASSERT(test, result == CMN_SUCCESS);
	}

	for (int32_t i = 0; i < 256; i++) {
		TEST_ASSERT(test, arr[i] == i);
	}
}

void checkForExponentialArrayMemoryCoherency(Test* test) {
	CmnResult result;

	uint8_t* memory = (uint8_t*)malloc(1024 * sizeof(int32_t));
	if (memory == nullptr) {
		testOutOfMemory(test);
	}

	CmnArena arena = cmnCreateArena(memory, 1024 * sizeof(int32_t));

	CmnExponentialArray<int32_t> arr;
	cmnCreateExponentialArray(&arr, &arena, &result);
	TEST_ASSERT(test, result == CMN_SUCCESS);

	for (int32_t i = 0; i < 256; i++) {
		cmnAppend(&arr, i, &result);
		TEST_ASSERT(test, result == CMN_SUCCESS);
	}

	int32_t* int32Memory = (int32_t*)memory;
	for (int32_t i = 0; i < 256; i++) {
		TEST_ASSERT(test, int32Memory[i] == i);
	}
}

