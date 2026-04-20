#ifndef MTL4_DELETION_MANAGER_H
#define MTL4_DELETION_MANAGER_H

#include <lib/common/page.h>
#include <lib/common/exponential_array.h>
#include <lib/common/mutex.h>

#include <lib/metal4/allocation.h>

typedef struct Mtl4DeletionManager {
	CmnPage		page;
	CmnArena	arena;

	// Locked by allocationsMutex
	size_t		bytesToDeallocate;
	// Locked by allocationsMutex
	CmnExponentialArray	<Mtl4AllocationHandle>	allocations;
	CmnMutex	allocationsMutex;

	// Locked by texturesMutex
	size_t		texturesToDeallocate;
	// Locked by texturesMutex
	CmnExponentialArray	<Mtl4Texture>	textures;
	CmnMutex	texturesMutex;
} Mtl4DeletionManager;
extern Mtl4DeletionManager gMtl4DeletionManager;

void mtl4InitDeletionManager(GpuResult* result);
void mtl4FiniDeletionManager(void);

void mtl4ScheduleAllocationForDeletion(Mtl4AllocationHandle allocation);
void mtl4ScheduleTextureForDeletion(Mtl4Texture texture);

bool mtl4ShouldDeleteScheduledResources(void);
bool mtl4ShouldDeleteScheduledAllocations(void);
bool mtl4ShouldDeleteScheduledTextures(void);

void mtl4DeleteScheduledResources(void);
void mtl4DeleteScheduledAllocations(void);
void mtl4DeleteScheduledTextures(void);

#endif // MTL4_DELETION_MANAGER_H

