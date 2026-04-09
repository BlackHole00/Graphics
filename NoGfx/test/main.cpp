#include "test.h"
#include "test.cpp"

#include "page.cpp"
#include "arena.cpp"
#include "exponential_array.cpp"
#include "pool.cpp"
#include "handle_map.cpp"
#include "btree.cpp"

#include "gpu.cpp"

TestRecord gCommonTests[] = {
	{ "Can access page memory",					canAccessPageMemory			},

	{ "Check for arena memory coherency",				checkForArenaMemoryCoherency		},

	{ "Check for exponential array data coherency",			checkForExponentialArrayDataCoherency	},
	{ "Check for exponential array memory coherency",		checkForExponentialArrayMemoryCoherency	},

	{ "Check for pool initial memory setup",			checkForPoolInitialMemorySetup		},
	{ "Check for block reusage in pools",				checkPoolBlockReusage			},
	{ "Check pool out of memory behaviour",				checkPoolOOMBehaviour			},
	{ "Check for pool behavious with uninitialized locations",	checkPoolUninitializedLocations		},

	{ "Check for handle map data coherency",			checkForHandleMapDataCoherency		},
	{ "Check for handle map bucket reusage",			checkForHandleMapBucketReusage		},
	{ "Check for handle map behaviour on generation overflow",	checkForHandleMapGenerationOverflowBehaviour },
	{ "Check for handle map behaviour on index overflow",		checkForHandleMapIndexOverflowBehaviour	},
	{ "Check for handle map behaviour on invalid handles",		checkForHandleMapInvalidHandleBehaviour	},

	{ "Check B-tree creation and initial root state",		checkBTreeCreation			},
	{ "Check B-tree insertion of keys and contains functionality",	checkBTreeInsertAndContains		},
	{ "Check B-tree get functionality with found and default element", checkBTreeGet			},
	{ "Check B-tree removal of keys from leaf nodes",		checkBTreeRemoveLeaf			},
	{ "Check B-tree removal of keys from non-leaf/internal nodes",	checkBTreeRemoveNonLeaf			},
	{ "Check B-tree root split when inserting enough keys",		checkBTreeRootSplit			},
	{ "Check B-tree predecessor and successor key retrieval",	checkBTreePredecessorSuccessor		},
};

TestRecord gNoGfxTests[] = {
	{ "Check GPU initialization and deinitialization",		checkGpuInitAndDeinit			},
	{ "Check GPU invalid backend handling",				checkGpuInvalidBackend			},
	{ "Check GPU device enumeration",				checkGpuEnumerateDevices		},
	{ "Check GPU device selection",					checkGpuSelectDevice			},
	{ "Check GPU invalid device selection",				checkGpuSelectInvalidDevice		},
	{ "Check GPU double device selection handling",			checkGpuDoubleDeviceSelection		},
	{ "Check GPU memory allocation and free",			checkGpuMallocAndFree			},
	{ "Check GPU free with invalid pointer",			checkGpuFreeInvalidPointer		},
	{ "Check GPU host to device pointer mapping",			checkGpuHostToDevicePointer		},
	{ "Check GPU host to device pointer mapping with offset",	checkGpuHostToDevicePointerWithOffset	},
};

int main(void) {
	size_t commonTestCount = sizeof(gCommonTests) / sizeof(*gCommonTests);
	doTests("Common utilities", gCommonTests, commonTestCount);

	size_t noGfxTestCount = sizeof(gNoGfxTests) / sizeof(*gNoGfxTests);
	doTests("No Graphics", gNoGfxTests, noGfxTestCount);

	return 0;
}

