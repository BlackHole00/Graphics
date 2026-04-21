#include "test.h"
#include "test.cpp"

#include "page.cpp"
#include "arena.cpp"
#include "synchronization.cpp"
#include "pointer_map.cpp"
#include "hash_map.cpp"
#include "exponential_array.cpp"
#include "pool.cpp"
#include "heap_allocator.cpp"
#include "handle_map.cpp"
#include "btree.cpp"
#include "storage_sync.cpp"

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

	{ "Check heap raw allocation is zeroed",			checkHeapRawAllocationIsZeroed		},
	{ "Check heap typed allocation overloads",			checkHeapTypedAllocationOverloads		},
	{ "Check heap raw realloc preserves and zeros",			checkHeapRawReallocPreservesAndZeros	},
	{ "Check heap typed realloc preserves and zeros",		checkHeapTypedReallocPreservesAndZeros	},
	{ "Check heap aligned raw allocation",				checkHeapAlignedRawAllocation		},

	{ "Check for handle map data coherency",			checkForHandleMapDataCoherency		},
	{ "Check for handle map bucket reusage",			checkForHandleMapBucketReusage		},
	{ "Check for handle map behaviour on generation overflow",	checkForHandleMapGenerationOverflowBehaviour	},
	{ "Check for handle map behaviour on index overflow",		checkForHandleMapIndexOverflowBehaviour	},
	{ "Check for handle map behaviour on invalid handles",		checkForHandleMapInvalidHandleBehaviour	},

	{ "Check B-tree creation and initial root state",		checkBTreeCreation			},
	{ "Check B-tree insertion of keys and contains functionality",	checkBTreeInsertAndContains		},
	{ "Check B-tree get functionality with found and default element", checkBTreeGet			},
	{ "Check B-tree removal of keys from leaf nodes",		checkBTreeRemoveLeaf			},
	{ "Check B-tree removal of keys from non-leaf/internal nodes",	checkBTreeRemoveNonLeaf			},
	{ "Check B-tree root split when inserting enough keys",		checkBTreeRootSplit			},
	{ "Check B-tree predecessor and successor key retrieval",	checkBTreePredecessorSuccessor		},

	{ "Check pointer map creation and initial state",		checkPointerMapCreation			},
	{ "Check pointer map insert and contains",			checkPointerMapInsertAndContains	},
	{ "Check pointer map get with found and default",		checkPointerMapGet			},
	{ "Check pointer map removal of keys",				checkPointerMapRemove			},
	{ "Check pointer map reserve and rehash",			checkPointerMapReserveAndRehash		},

	{ "Check hash map creation and initial state",			checkHashMapCreation			},
	{ "Check hash map insert contains and get",			checkHashMapInsertContainsAndGet		},
	{ "Check hash map overwrite does not grow length",		checkHashMapOverwriteDoesNotGrowLength	},
	{ "Check hash map remove and reuse deleted slots",		checkHashMapRemoveAndReuseDeletedSlots	},
	{ "Check hash map reserve and rehash",			checkHashMapReserveAndRehash		},

	{ "Check mutex mutual exclusion with pthreads",			checkMutexMutualExclusionWithPthreads	},
	{ "Check mutex try-lock while locked",				checkMutexTryLockWhileLocked		},
	{ "Check condition signal wakes waiter",			checkConditionSignalWakesWaiter		},
	{ "Check condition wait timeout",				checkConditionWaitTimeout		},
	{ "Check RW mutex allows concurrent readers",			checkRWMutexAllowsConcurrentReaders	},
	{ "Check RW mutex write exclusion",				checkRWMutexWriteExclusion		},

	{ "Check storage sync acquire/release for valid handles", 	checkStorageSyncAcquireAndReleaseValidHandle	},
	{ "Check storage sync invalid handles do not increment users", 	checkStorageSyncInvalidHandleDoesNotIncrementUsers	},
	{ "Check storage sync deletion waits for active users", 	checkStorageSyncDeletionWaitsForActiveUsers	},
};

TestRecord gNoGfxTests[] = {
	{ "Check GPU initialization and deinitialization",		checkGpuInitAndDeinit			},
	{ "Check GPU invalid backend handling",				checkGpuInvalidBackend			},
	{ "Check GPU device enumeration",				checkGpuEnumerateDevices		},
	{ "Check GPU device selection",					checkGpuSelectDevice			},
	{ "Check GPU invalid device selection",				checkGpuSelectInvalidDevice		},
	{ "Check GPU double device selection handling",			checkGpuDoubleDeviceSelection		},
	{ "Check GPU memory allocation and free",			checkGpuMallocAndFree			},
	{ "Check GPU memory allocation and free for GPU-only memory",	checkGpuMallocAndFreeGpuMemory		},
	{ "Check GPU free with invalid pointer",			checkGpuFreeInvalidPointer		},
	{ "Check GPU host to device pointer mapping",			checkGpuHostToDevicePointer		},
	{ "Check GPU host to device pointer fails for GPU-only memory",	checkGpuHostToDevicePointerOnGpuMemory	},
	{ "Check GPU host to device pointer mapping with offset",	checkGpuHostToDevicePointerWithOffset	},
	{ "Check GPU texture size and alignment calculation", 		checkGpuTextureSizeAlign		},
	{ "Check GPU texture size and alignment invalid descriptor", 	checkGpuTextureSizeAlignInvalidDesc	},
	{ "Check GPU texture creation", 				checkGpuCreateTexture			},
	{ "Check GPU texture creation on CPU allocation", 		checkGpuCreateTextureOnCpuAllocation	},
	{ "Check GPU texture creation invalid descriptor", 		checkGpuCreateTextureInvalidDesc	},
	{ "Check GPU texture view descriptor creation", 		checkGpuTextureViewDescriptor		},
	{ "Check GPU RW texture view descriptor creation", 		checkGpuRWTextureViewDescriptor		},
	{ "Check GPU texture view descriptor invalid texture", 		checkGpuTextureViewDescriptorInvalidTexture	},
	{ "Check GPU texture view descriptor invalid descriptor", 	checkGpuTextureViewDescriptorInvalidDesc	},
	{ "Check GPU compute pipeline creation", 			checkGpuCreateComputePipeline		},
	{ "Check GPU compute pipeline invalid IR handling", 		checkGpuCreateComputePipelineInvalidIr	},
	{ "Check GPU render pipeline creation", 			checkGpuCreateRenderPipeline		},
	{ "Check GPU meshlet pipeline creation", 			checkGpuCreateMeshletPipeline		},
	{ "Check GPU allocation create/destroy across threads", 	checkGpuAllocationCreatedAndDestroyedOnDifferentThreads },
	{ "Check GPU texture create/destroy across threads", 		checkGpuTextureCreatedAndBackingFreedOnDifferentThreads },
};

int main(void) {
	size_t commonTestCount = sizeof(gCommonTests) / sizeof(*gCommonTests);
	doTests("Common utilities", gCommonTests, commonTestCount);

	size_t noGfxTestCount = sizeof(gNoGfxTests) / sizeof(*gNoGfxTests);
	doTests("No Graphics", gNoGfxTests, noGfxTestCount);

	return 0;
}

