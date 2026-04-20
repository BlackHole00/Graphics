#include "deletion_manager.h"

Mtl4DeletionManager gMtl4DeletionManager;

void mtl4InitDeletionManager(GpuResult* result) {
	CmnResult localResult;

	gMtl4DeletionManager.page = cmnCreatePage(1024 * 1024, CMN_PAGE_READABLE | CMN_PAGE_WRITABLE, &localResult);
	if (localResult != CMN_SUCCESS) {
		CMN_SET_RESULT(result, GPU_OUT_OF_CPU_MEMORY);
		return;
	}

	gMtl4DeletionManager.arena = cmnPageToArena(gMtl4DeletionManager.page);
	CmnAllocator arenaAllocator = cmnArenaAllocator(&gMtl4DeletionManager.arena);

	cmnCreateExponentialArray(&gMtl4DeletionManager.allocations, arenaAllocator, &localResult);
	if (localResult != CMN_SUCCESS) {
		CMN_SET_RESULT(result, GPU_OUT_OF_CPU_MEMORY);
		return;
	}

	cmnCreateExponentialArray(&gMtl4DeletionManager.textures, arenaAllocator, &localResult);
	if (localResult != CMN_SUCCESS) {
		CMN_SET_RESULT(result, GPU_OUT_OF_CPU_MEMORY);
		return;
	}

	CMN_SET_RESULT(result, GPU_SUCCESS);
}

void mtl4FiniDeletionManager(void) {
	cmnDestroyPage(gMtl4DeletionManager.page);

	gMtl4DeletionManager = {};
}

void mtl4ScheduleAllocationForDeletion(Mtl4AllocationHandle allocation) {
	Mtl4AllocationMetadata* metadata = mtl4AcquireAllocationMetadataFrom(allocation, nullptr);
	if (metadata == nullptr) {
		return;
	}
	defer (mtl4ReleaseAllocationMetadata());

	CmnScopedMutex guard(&gMtl4DeletionManager.allocationsMutex);
	cmnAppend(&gMtl4DeletionManager.allocations, allocation, nullptr);

	// TODO: Not actually accurate, since it could be more with the alignment.
	gMtl4DeletionManager.bytesToDeallocate += metadata->size;
}

void mtl4ScheduleTextureForDeletion(Mtl4Texture texture) {
	CmnScopedMutex guard(&gMtl4DeletionManager.texturesMutex);
	cmnAppend(&gMtl4DeletionManager.textures, texture, nullptr);
}

bool mtl4ShouldDeleteScheduledResources(void) {
	return mtl4ShouldDeleteScheduledTextures() || mtl4ShouldDeleteScheduledAllocations();
}

bool mtl4ShouldDeleteScheduledAllocations(void) {
	return gMtl4DeletionManager.bytesToDeallocate >= 10 * 1024 * 1024;
}

bool mtl4ShouldDeleteScheduledTextures(void) {
	return gMtl4DeletionManager.texturesToDeallocate >= 128;
}

void mtl4DeleteScheduledResources(void) {
	mtl4DeleteScheduledAllocations();
	mtl4DeleteScheduledTextures();
}

void mtl4DeleteScheduledAllocations(void) {
	CmnScopedStorageSyncDeletionLock guard(&gMtl4AllocationStorage.sync);

	for (size_t i = 0; i < gMtl4DeletionManager.allocations.length; i++) {
		mtl4PhisicallyDestroyAllocation(gMtl4DeletionManager.allocations[i]);
	}

	cmnResize(&gMtl4DeletionManager.allocations, 0, nullptr);
}

void mtl4DeleteScheduledTextures(void) {
	CmnScopedStorageSyncDeletionLock guard(&gMtl4TextureStorage.sync);

	for (size_t i = 0; i < gMtl4DeletionManager.textures.length; i++) {
		mtl4PhisicallyDestroyTexture(gMtl4DeletionManager.textures[i]);
	}

	cmnResize(&gMtl4DeletionManager.textures, 0, nullptr);
}

