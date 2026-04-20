#include "test.h"

#include <pthread.h>
#include <sched.h>
#include <stdlib.h>

#include <lib/common/arena.h>
#include <lib/common/atomic.h>
#include <lib/common/storage_sync.h>

void checkStorageSyncAcquireAndReleaseValidHandle(Test* test) {
	CmnResult result;

	uint8_t* memory = (uint8_t*)malloc(1024 * sizeof(int32_t));
	if (memory == nullptr) {
		testOutOfMemory(test);
	}

	CmnArena arena = cmnCreateArena(memory, 1024 * sizeof(int32_t), true);
	CmnAllocator arenaAllocator = cmnArenaAllocator(&arena);

	CmnHandleMap<int32_t> map;
	cmnCreateHandleMap(&map, arenaAllocator, -1, &result);
	TEST_ASSERT(test, result == CMN_SUCCESS);

	CmnHandle handle = cmnInsert(&map, 42, &result);
	TEST_ASSERT(test, result == CMN_SUCCESS);

	CmnStorageSync sync = {};

	bool wasHandleValid = false;
	int32_t& value = cmnStorageSyncAcquireResource(&map, &sync, handle, &wasHandleValid);
	TEST_ASSERT(test, wasHandleValid);
	TEST_ASSERT(test, value == 42);
	TEST_ASSERT(test, cmnAtomicLoad(&sync.users.value, CMN_ACQUIRE) == 1u);

	cmnStorageSyncReleaseResource(&sync);
	TEST_ASSERT(test, cmnAtomicLoad(&sync.users.value, CMN_ACQUIRE) == 0u);
}

void checkStorageSyncInvalidHandleDoesNotIncrementUsers(Test* test) {
	CmnResult result;

	uint8_t* memory = (uint8_t*)malloc(1024 * sizeof(int32_t));
	if (memory == nullptr) {
		testOutOfMemory(test);
	}

	CmnArena arena = cmnCreateArena(memory, 1024 * sizeof(int32_t), true);
	CmnAllocator arenaAllocator = cmnArenaAllocator(&arena);

	CmnHandleMap<int32_t> map;
	cmnCreateHandleMap(&map, arenaAllocator, -1, &result);
	TEST_ASSERT(test, result == CMN_SUCCESS);

	CmnStorageSync sync = {};

	bool wasHandleValid = true;
	int32_t& value = cmnStorageSyncAcquireResource(&map, &sync, CmnHandle{123u, 456u}, &wasHandleValid);
	TEST_ASSERT(test, !wasHandleValid);
	TEST_ASSERT(test, value == -1);
	TEST_ASSERT(test, cmnAtomicLoad(&sync.users.value, CMN_ACQUIRE) == 0u);
}

typedef struct StorageSyncUserContext {
	CmnStorageSync* sync;
	CmnHandleMap<int32_t>* map;
	CmnHandle handle;

	uint32_t acquired;
	uint32_t releaseUser;
} StorageSyncUserContext;

static void* storageSyncUserThreadProc(void* ptr) {
	StorageSyncUserContext* context = (StorageSyncUserContext*)ptr;

	bool wasHandleValid = false;
	int32_t& value = cmnStorageSyncAcquireResource(context->map, context->sync, context->handle, &wasHandleValid);
	if (!wasHandleValid) {
		return nullptr;
	}

	(void)value;
	cmnAtomicStore(&context->acquired, 1u, CMN_RELEASE);
	while (cmnAtomicLoad(&context->releaseUser, CMN_ACQUIRE) == 0u) {
		sched_yield();
	}

	cmnStorageSyncReleaseResource(context->sync);
	return nullptr;
}

typedef struct StorageSyncDeletionContext {
	CmnStorageSync* sync;
	uint32_t entered;
} StorageSyncDeletionContext;

static void* storageSyncDeletionThreadProc(void* ptr) {
	StorageSyncDeletionContext* context = (StorageSyncDeletionContext*)ptr;

	cmnStorageSyncDeletionLock(context->sync);
	cmnAtomicStore(&context->entered, 1u, CMN_RELEASE);
	cmnStorageSyncDeletionUnlock(context->sync);

	return nullptr;
}

void checkStorageSyncDeletionWaitsForActiveUsers(Test* test) {
	CmnResult result;

	uint8_t* memory = (uint8_t*)malloc(1024 * sizeof(int32_t));
	if (memory == nullptr) {
		testOutOfMemory(test);
	}

	CmnArena arena = cmnCreateArena(memory, 1024 * sizeof(int32_t), true);
	CmnAllocator arenaAllocator = cmnArenaAllocator(&arena);

	CmnHandleMap<int32_t> map;
	cmnCreateHandleMap(&map, arenaAllocator, -1, &result);
	TEST_ASSERT(test, result == CMN_SUCCESS);

	CmnHandle handle = cmnInsert(&map, 7, &result);
	TEST_ASSERT(test, result == CMN_SUCCESS);

	CmnStorageSync sync = {};

	StorageSyncUserContext userContext = {};
	userContext.sync = &sync;
	userContext.map = &map;
	userContext.handle = handle;

	pthread_t userThread;
	int createResult = pthread_create(&userThread, nullptr, storageSyncUserThreadProc, &userContext);
	TEST_ASSERT(test, createResult == 0);

	while (cmnAtomicLoad(&userContext.acquired, CMN_ACQUIRE) == 0u) {
		sched_yield();
	}

	StorageSyncDeletionContext deletionContext = {};
	deletionContext.sync = &sync;

	pthread_t deletionThread;
	createResult = pthread_create(&deletionThread, nullptr, storageSyncDeletionThreadProc, &deletionContext);
	TEST_ASSERT(test, createResult == 0);

	for (size_t i = 0; i < 1000; i++) {
		if (cmnAtomicLoad(&deletionContext.entered, CMN_ACQUIRE) != 0u) {
			break;
		}
		sched_yield();
	}
	TEST_ASSERT(test, cmnAtomicLoad(&deletionContext.entered, CMN_ACQUIRE) == 0u);

	cmnAtomicStore(&userContext.releaseUser, 1u, CMN_RELEASE);

	int joinResult = pthread_join(userThread, nullptr);
	TEST_ASSERT(test, joinResult == 0);

	joinResult = pthread_join(deletionThread, nullptr);
	TEST_ASSERT(test, joinResult == 0);
	TEST_ASSERT(test, cmnAtomicLoad(&deletionContext.entered, CMN_ACQUIRE) == 1u);
	TEST_ASSERT(test, cmnAtomicLoad(&sync.users.value, CMN_ACQUIRE) == 0u);
	TEST_ASSERT(test, (CmnStorageSyncState)cmnAtomicLoad(&sync.state.value, CMN_ACQUIRE) == CMN_STORAGE_SYNC_NORMAL);
}
