#ifndef GPU_POINTERRANGETREE_H
#define GPU_POINTERRANGETREE_H

#include <gpu/gpu.h>
#include <Metal/Metal.h>

#include <lib/common/page.h>
#include <lib/common/btree.h>
#include <lib/common/pointer_map.h>
#include <lib/common/element_pool.h>
#include <lib/common/mutex.h>
#include <lib/metal4/textures.h>


typedef struct Mtl4AddressRange {
	uintptr_t	start;
	size_t		length;
} Mtl4AddressRange;

typedef struct Mtl4GpuAddress {
	bool		guard			: 1; /** Always true for gpu addresses. */
	uint32_t	allocationIdentifier	: 23;
	uint64_t	offset			: 40;
} Mtl4GpuAddress;
static_assert(sizeof(Mtl4GpuAddress) == sizeof(uintptr_t), "The size of the virtual address must be the same as a pointer.");

inline Mtl4GpuAddress mtl4PtrToGpuAddress(void* ptr) {
	return *(Mtl4GpuAddress*)&ptr;
}

inline void* mtl4GpuAddressToPtr(Mtl4GpuAddress address) {
	return *(void**)&address;
}

#define MTL4_TEXTURES_PER_ALLOCATION_TEXTURE_BUCKET 7
typedef struct Mtl4AllocationTextures {
	Mtl4Texture textures[MTL4_TEXTURES_PER_ALLOCATION_TEXTURE_BUCKET];

	struct Mtl4AllocationTextures* nextBucket;
} Mtl4AllocationTextures;
static_assert(sizeof(Mtl4AllocationTextures) == 64, "The allocation metadata pool should be able to contain this struct.");

typedef struct Mtl4AllocationMetadata {
	GpuMemory	memory;
	size_t		size;
	size_t		align;

	// Might be nil if memory == GPU_MEMORY_GPU and the actual memory has not yet been committed.
	id<MTLBuffer>	buffer;

	void*		cpuAddress;
	Mtl4GpuAddress	gpuAddress;

	id<MTLHeap>	associatedTextureHeap;
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
	CmnPage		miscArenaPage;

	CmnPool		allocationMetadataPool;
	CmnPool		addressRangeMapNodesPool;
	CmnArena	miscArena;

	// Contains a direct mapping for addresses of GPU_MEMORY_DEFAULT or GPU_MEMORY_READBACK allocations.
	// Used for fast lookups, but does not support addresses with offsets.
	// Contains both _cpu mapped gpu virtual addresses_ and _gpu virtual addresses_.
	CmnPointerMap	<Mtl4AllocationMetadata*>			cpuAllocationMap;
	// Contains a generic mapping for addresses of GPU_MEMORY_DEFAULT or GPU_MEMORY_READBACK allocations.
	// Used for slow lookups, whilst supporting addresses with offsets.
	// Contains only _cpu mapped gpu virtual addresses_ ranges.
	CmnBTree	<Mtl4AddressRange, Mtl4AllocationMetadata*>	cpuAddressRangeMap;
	// Contains a generic mapping for addresses of GPU_MEMORY_DEFAULT or GPU_MEMORY_READBACK allocations.
	// Contains only _gpu virtual addresses_ ranges.
	CmnElementPool	<Mtl4AllocationMetadata*>			gpuAllocationPool;
	CmnMutex	mutex;
} Mtl4AllocationStorage;
extern Mtl4AllocationStorage gMtl4AllocationStorage;

void mtl4InitAllocationStorage(GpuResult* result);
void mtl4FiniAllocationStorage(void);

void* mtl4Malloc(size_t size, size_t align, GpuMemory memory, GpuResult* result);
void  mtl4Free(void* ptr);
void* mtl4HostToDevicePointer(void* ptr, GpuResult* result);

id<MTLBuffer> mtl4AllocateBuffer(size_t size, size_t align, GpuMemory memory, GpuResult* result);

inline bool mtl4IsCpuAddress(void* ptr) {
	Mtl4GpuAddress address = mtl4PtrToGpuAddress(ptr);
	return address.guard == 0;
}

inline bool mtl4IsGpuAddress(void* ptr) {
	Mtl4GpuAddress address = mtl4PtrToGpuAddress(ptr);
	return address.guard == 1;
}

uintptr_t mtl4GpuAddressToActual(void* gpuPtr);

// Acquires the metadata of any non private allocation.
// NOTE: Thread unsafe: requires locking gMtl4AllocationStorage.mutex
Mtl4AllocationMetadata* mtl4GetAllocationMetadataOf(void* ptr, bool attemptRangeBasedLookup);

// Acquires the metadata of a _cpu mapped virtual address_ related to a non-private allocation.
// NOTE: Thread unsafe: requires locking gMtl4AllocationStorage.mutex
Mtl4AllocationMetadata* mtl4GetAllocationMetadataOfCpuPtr(void* ptr, bool attemptRangeBasedLookup);

// Acquires the metadata of a _gpu virtual address_ related to a non-private allocation.
// NOTE: Thread unsafe: requires locking gMtl4AllocationStorage.mutex
Mtl4AllocationMetadata* mtl4GetAllocationMetadataOfGpuPtr(Mtl4GpuAddress address);
inline Mtl4AllocationMetadata* mtl4GetAllocationMetadataOfGpuPtr(void* ptr) {
	return mtl4GetAllocationMetadataOfGpuPtr(mtl4PtrToGpuAddress(ptr));
}

void mtl4AssociateTextureToAllocation(Mtl4AllocationMetadata* metadata, Mtl4Texture texture, GpuResult* result);
void mtl4FreeAssociatedTextures(Mtl4AllocationTextures* textureBucket);

void mtl4EnsureBackingBufferIsAllocated(Mtl4GpuAddress address, GpuResult* result);

// NOTE: This is an HACK, since eq and cmp are not symmetrical. This works because the implementation of BTree always
//	compares keys and values with the same order: keys on the right, values on the left.
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

#endif

