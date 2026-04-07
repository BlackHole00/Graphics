#include "test.h"
#include "test.cpp"

#include "page.cpp"
#include "arena.cpp"
#include "exponential_array.cpp"

TestRecord gTests[] = {
	{ "Can access page memory",				canAccessPageMemory			},
	{ "Check for arena memory coherency",			checkForArenaMemoryCoherency		},
	{ "Check for exponential array data coherency",		checkForExponentialArrayDataCoherency	},
	{ "Check for exponential array memory coherency",	checkForExponentialArrayMemoryCoherency	},
};

int main(void) {
	size_t testCount = sizeof(gTests) / sizeof(*gTests);
	doTests(gTests, testCount);

	return 0;
}

