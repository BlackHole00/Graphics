#include "test.h"
#include "test.cpp"

#include "page.cpp"
#include "arena.cpp"
#include "exponential_array.cpp"
#include "pool.cpp"

TestRecord gTests[] = {
	{ "Can access page memory",				canAccessPageMemory			},

	{ "Check for arena memory coherency",			checkForArenaMemoryCoherency		},

	{ "Check for exponential array data coherency",		checkForExponentialArrayDataCoherency	},
	{ "Check for exponential array memory coherency",	checkForExponentialArrayMemoryCoherency	},

	{ "Check for pool initial memory setup",		checkForPoolInitialMemorySetup		},
	{ "Check for block reusage in pools",			checkPoolBlockReusage			},
	{ "Check pool out of memory behaviour", 		checkPoolOOMBehaviour			},
	{ "Check for pool behavious with uninitialized locations",	checkPoolUninitializedLocations	},
};

int main(void) {
	size_t testCount = sizeof(gTests) / sizeof(*gTests);
	doTests(gTests, testCount);

	return 0;
}

