#ifndef GPU_POINTERRANGETREE_H
#define GPU_POINTERRANGETREE_H

#include <gpu/gpu.h>
#include <Metal/Metal.h>

#include <lib/common/page.h>
#include <lib/common/btree.h>
#include <lib/common/pointer_map.h>
#include <lib/common/element_pool.h>
#include <lib/common/storage_sync.h>
#include <lib/metal4/textures.h>


typedef CmnHandle Mtl4AllocationHandle;

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

#define MTL4_TEXTURES_PER_ALLOCATION_TEXTURE_BUCKET 11
typedef struct Mtl4AllocationTextures {
	Mtl4Texture textures[MTL4_TEXTURES_PER_ALLOCATION_TEXTURE_BUCKET];

	struct Mtl4AllocationTextures* nextBucket;
} Mtl4AllocationTextures;
static_assert(sizeof(Mtl4AllocationTextures) <= 96, "The allocation misc pool should be able to contain this struct.");

// typedef enum Mtl4InternalAllocationUsage {
// 	// The allocation references directly a `MTLBuffer`. Its memory type is _GPU_MEMORY_DEFAULT_ or _GPU_MEMORY_READBACK_.
// 	MTL4_ALLOCATION_DIRECT			= 1,
// 	// The allocation is _GPU_MEMORY_GPU_. No actual memory will be committed until first usage, thus the allocation is _virtual_.
// 	MTL4_ALLOCATION_VIRTUAL			= 2,
	
// 	// The allocation has a real backing `MTLBuffer`.
// 	MTL4_ALLOCATION_COMMITTED		= 4,

// 	// The allocation has been used for a single texture. The allocation does not have any other free memory.
// 	MTL4_ALLOCATION_FOR_SINGLE_TEXTURE	= 8,

// 	// The allocation is correlated with a texture heap, thus can contain multiple textures, given there is space in the heap.
// 	MTL4_ALLOCATION_FOR_TEXTURE_HEAP	= 16,
// } Mtl4InternalAllocationUsage;
// typedef uint32_t Mtl4InternalAllocationUsages;

typedef struct Mtl4AllocationMetadata {
	// Final
	GpuMemory			memory;

	// Atomic, settable once
	bool		scheduledForDeletion;
	// Mtl4InternalAllocationUsages	internalUsage;

	// Final
	size_t		size;

	// TODO: Shouldn't be needed here.
	// Final
	size_t		align;

	// Final
	Mtl4GpuAddress	assignedGpuAddress;

	// NOTE: Might be nil if memory == GPU_MEMORY_GPU and the actual memory has not yet been committed.
	// Atomic, settable once
	id<MTLBuffer>	buffer;

	// Atomic, settable once
	id<MTLHeap>	associatedTextureHeap;

	// Locked by relatedTexturesMutex
	Mtl4AllocationTextures*	relatedTextures;
	CmnRWMutex		relatedTexturesMutex;
} Mtl4AllocationMetadata;

// Mtl4AllocationMetadata and Mtl4AllocationTextures should be allocated both using gMtl4AllocationStorage.
#define MTL4_ALLOCATION_METADATA_OBJECT_SIZE sizeof(Mtl4AllocationTextures)
static_assert(
	sizeof(Mtl4AllocationMetadata) <= 96,
	"Mtl4AllocationMetadata is too big for the allocation misc pool"
);

typedef struct Mtl4AllocationStorage {
	CmnPage		addressRangeMapPage;
	CmnPage		miscPoolPage;
	CmnPage		miscArenaPage;

	CmnPool		addressRangeMapNodesPool;
	CmnPool		miscPool;
	CmnArena	miscArena;

	// Contains a direct mapping for addresses of GPU_MEMORY_DEFAULT or GPU_MEMORY_READBACK allocations.
	// Used for fast lookups, but does not support addresses with offsets.
	CmnPointerMap	<Mtl4AllocationHandle>				cpuAllocationMap;
	// Contains a generic mapping for addresses of GPU_MEMORY_DEFAULT or GPU_MEMORY_READBACK allocations.
	// Used for slow lookups, whilst supporting addresses with offsets.
	CmnBTree	<Mtl4AddressRange, Mtl4AllocationHandle>	cpuAddressRangeMap;
	// Contains a generic mapping for gpu addresses.
	CmnElementPool	<Mtl4AllocationHandle>				gpuAllocationMap;

	CmnHandleMap	<Mtl4AllocationMetadata>	allocations;
	CmnStorageSync sync;
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
inline size_t mtl4GpuAddressOffsetFromBase(void* gpuPtr) {
	Mtl4GpuAddress address = mtl4PtrToGpuAddress(gpuPtr);
	return address.offset;
}

uintptr_t mtl4GpuAddressToActual(void* gpuPtr);

Mtl4AllocationMetadata* mtl4AcquireAllocationMetadataFrom(Mtl4AllocationHandle handle, bool* wasHandleValid);
void mtl4ReleaseAllocationMetadata(void);

Mtl4AllocationHandle mtl4AllocationHandleOf(void* ptr, bool attemptRangeBasedLookup, bool* couldFindMetadata);
Mtl4AllocationHandle mtl4AllocationHandleOfCpuPtr(void* ptr, bool attemptRangeBasedLookup, bool* couldFindMetadata);
Mtl4AllocationHandle mtl4AllocationHandleOfGpuPtr(Mtl4GpuAddress address, bool* couldFindMetadata);
inline Mtl4AllocationHandle mtl4AllocationHandleOfGpuPtr(void* ptr, bool* couldFindMetadata) {
	return mtl4AllocationHandleOfGpuPtr(mtl4PtrToGpuAddress(ptr), couldFindMetadata);
}

Mtl4AllocationMetadata* mtl4AcquireAllocationMetadataFrom(void* ptr, bool attemptRangeBasedLookup);
Mtl4AllocationMetadata* mtl4AcquireAllocationMetadataFromCpuPtr(void* ptr, bool attemptRangeBasedLookup);
Mtl4AllocationMetadata* mtl4AcquireAllocationMetadataFromGpuPtr(Mtl4GpuAddress address);
inline Mtl4AllocationMetadata* mtl4AcquireAllocationMetadataFromGpuPtr(void* ptr) {
	return mtl4AcquireAllocationMetadataFromGpuPtr(mtl4PtrToGpuAddress(ptr));
}

inline void* mtl4CpuAddressOf(Mtl4AllocationMetadata* metadata) {
	return metadata->buffer.contents;
}

void mtl4AssociateTextureToAllocation(Mtl4AllocationMetadata* metadata, Mtl4Texture texture, GpuResult* result);
// NOTE: Not thread safe. Requires external locking.
void mtl4FreeAssociatedTextures(Mtl4AllocationMetadata* metadata);

void mtl4EnsureBackingBufferIsAllocated(Mtl4GpuAddress address, GpuResult* result);
bool mtl4IsScheduledForDeletion(void* ptr);

// NOTE: Requires a deletion lock in gMtl4AllocationStorage.sync
void mtl4DestroyAllocation(Mtl4AllocationHandle handle);

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

