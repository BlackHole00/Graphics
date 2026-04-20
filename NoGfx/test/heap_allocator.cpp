#include "test.h"

#include <lib/common/heap_allocator.h>

void checkHeapRawAllocationIsZeroed(Test* test) {
	CmnResult result;
	uint8_t* data = (uint8_t*)cmnHeapAllocRaw(64, &result);

	TEST_ASSERT(test, result == CMN_SUCCESS);
	TEST_ASSERT(test, data != nullptr);

	for (size_t i = 0; i < 64; i++) {
		TEST_ASSERT(test, data[i] == 0);
	}

	cmnHeapFree(data);
}

void checkHeapTypedAllocationOverloads(Test* test) {
	CmnResult result;

	uint32_t* values = cmnHeapAlloc<uint32_t>(8, &result);
	TEST_ASSERT(test, result == CMN_SUCCESS);
	TEST_ASSERT(test, values != nullptr);
	for (size_t i = 0; i < 8; i++) {
		TEST_ASSERT(test, values[i] == 0);
	}

	uint32_t* single = cmnHeapAlloc<uint32_t>(&result);
	TEST_ASSERT(test, result == CMN_SUCCESS);
	TEST_ASSERT(test, single != nullptr);
	TEST_ASSERT(test, *single == 0);

	cmnHeapFree(values);
	cmnHeapFree(single);
}

void checkHeapRawReallocPreservesAndZeros(Test* test) {
	CmnResult result;

	uint8_t* data = (uint8_t*)cmnHeapAllocRaw(16, &result);
	TEST_ASSERT(test, result == CMN_SUCCESS);
	TEST_ASSERT(test, data != nullptr);

	for (size_t i = 0; i < 16; i++) {
		data[i] = (uint8_t)(i + 1);
	}

	data = (uint8_t*)cmnHeapReallocRaw(data, 16, 32, &result);
	TEST_ASSERT(test, result == CMN_SUCCESS);
	TEST_ASSERT(test, data != nullptr);

	for (size_t i = 0; i < 16; i++) {
		TEST_ASSERT(test, data[i] == (uint8_t)(i + 1));
	}
	for (size_t i = 16; i < 32; i++) {
		TEST_ASSERT(test, data[i] == 0);
	}

	cmnHeapFree(data);
}

void checkHeapTypedReallocPreservesAndZeros(Test* test) {
	CmnResult result;

	uint64_t* values = cmnHeapAlloc<uint64_t>(4, &result);
	TEST_ASSERT(test, result == CMN_SUCCESS);
	TEST_ASSERT(test, values != nullptr);

	for (size_t i = 0; i < 4; i++) {
		values[i] = i + 100;
	}

	values = cmnHeapRealloc<uint64_t>(values, 4, 8, &result);
	TEST_ASSERT(test, result == CMN_SUCCESS);
	TEST_ASSERT(test, values != nullptr);

	for (size_t i = 0; i < 4; i++) {
		TEST_ASSERT(test, values[i] == i + 100);
	}
	for (size_t i = 4; i < 8; i++) {
		TEST_ASSERT(test, values[i] == 0);
	}

	cmnHeapFree(values);
}

void checkHeapAlignedRawAllocation(Test* test) {
	CmnResult result;

	const size_t align = 32;
	const size_t size = 64;
	uint8_t* data = (uint8_t*)cmnHeapAllocRaw(size, align, &result);

	TEST_ASSERT(test, result == CMN_SUCCESS);
	TEST_ASSERT(test, data != nullptr);
	TEST_ASSERT(test, ((uintptr_t)data % align) == 0);

	cmnHeapFree(data);
}
