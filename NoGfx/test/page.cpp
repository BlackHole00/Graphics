#include "test.h"

#include <lib/common/page.h>

void canAccessPageMemory(Test* test) {
	CmnResult result;

	CmnPage page = cmnCreatePage(128 * 512, CMN_PAGE_READABLE | CMN_PAGE_WRITABLE, &result);
	TEST_ASSERT(test, result == CMN_SUCCESS);

	uint8_t* memoryBytes = (uint8_t*)page.baseAddress;
	for (size_t i = 0; i < 256; i++) {
		size_t offset = i * 128;
		memoryBytes[offset] = i;
	}

	for (size_t i = 0; i < 256; i++) {
		size_t offset = i * 128;
		TEST_ASSERT(test, memoryBytes[offset] == i);
	}

	cmnDestroyPage(page);
}

