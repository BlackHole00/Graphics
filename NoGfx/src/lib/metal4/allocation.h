#ifndef GPU_POINTERRANGETREE_H
#define GPU_POINTERRANGETREE_H

#include <gpu/gpu.h>
#include <Metal/Metal.h>

#include <lib/common/page.h>
#include <lib/common/btree.h>
#include <lib/common/pointer_map.h>
#include <lib/common/mutex.h>
#include <lib/metal4/textures.h>


typedef struct Mtl4AddressRange {
	uintptr_t	start;
	size_t		length;
} Mtl4AddressRange;

// NOTE: This is a TERRIBLE HACK, since eq and cmp are not symmetrical
template<>
struct CmnTypeTraits<Mtl4AddressRange> {
	static bool eq(const Mtl4AddressRange& left, const Mtl4AddressRange& right) {
		return right.start >= left.start &&
			right.start + right.length <= left.start + left.length;
	}

	static CmnCmp cmp(const Mtl4AddressRange& left, const Mtl4AddressRange& right) {
		if (right.start < left.start) {
			return CMN_MORE;
		}
		if (right.start + right.length > left.start + left.length) {
			return CMN_LESS;
		}
		return CMN_EQUALS;
	}
};

#define MTL4_TEXTURES_PER_ALLOCATION_TEXTURE_BUCKET 7
typedef struct Mtl4AllocationTextures {
	Mtl4Texture textures[MTL4_TEXTURES_PER_ALLOCATION_TEXTURE_BUCKET];

	struct Mtl4AllocationTextures* nextBucket;
} Mtl4AllocationTextures;
static_assert(
	sizeof(Mtl4AllocationTextures) == 64,
	"The allocation metadata pool should be able to contain this struct."
);

typedef struct Mtl4AllocationMetadata {
	id<MTLBuffer>	buffer;
	GpuMemory	memory;

	size_t		size;
	void*		cpuAddress;
	void*		gpuAddress;

	Mtl4AllocationTextures*	relatedTextures;
} Mtl4AllocationMetadata;

// Mtl4AllocationMetadata and Mtl4AllocationTextures should be allocated both using gMtl4AllocationStorage.
#define MTL4_ALLOCATION_METADATA_OBJECT_SIZE sizeof(Mtl4AllocationTextures)
static_assert(
	sizeof(Mtl4AllocationMetadata) <= sizeof(Mtl4AllocationTextures),
	"Mtl4AllocationMetadata is too big for the allocation metadata pool"
);

typedef struct Mtl4AllocationStorage {
	CmnPage		allocationMetadataPage;
	CmnPage		addressRangeMapPage;

	CmnPool		allocationMetadataPool;
	CmnPool		addressRangeMapNodesPool;

	CmnBTree<Mtl4AddressRange, Mtl4AllocationMetadata*> addressRangeMap;
	CmnPointerMap<Mtl4AllocationMetadata*> allocationMap;
	
	CmnMutex	mutex;
} Mtl4AllocationStorage;
extern Mtl4AllocationStorage gMtl4AllocationStorage;

void mtl4InitAllocationStorage(GpuResult* result);
void mtl4FiniAllocationStorage(void);

void* mtl4Malloc(size_t size, size_t align, GpuMemory memory, GpuResult* result);
void  mtl4Free(void* ptr);
void* mtl4HostToDevicePointer(void* ptr, GpuResult* result);

// NOTE: Thread unsafe: requires locking gMtl4AllocationStorage.allocationMutex
Mtl4AllocationMetadata* mtl4GetAllocationMetadataOf(void* ptr, bool attemptRangeBasedLookup);
void mtl4AssociateTextureToAllocation(Mtl4AllocationMetadata* metadata, Mtl4Texture texture, GpuResult* result);
void mtl4FreeAssociatedTextures(Mtl4AllocationTextures* textureBucket);

#endif

