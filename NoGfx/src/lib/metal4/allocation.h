#ifndef GPU_POINTERRANGETREE_H
#define GPU_POINTERRANGETREE_H

#include <gpu/gpu.h>
#include <Metal/Metal.h>

#include <lib/common/page.h>
#include <lib/common/btree.h>


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

typedef struct Mtl4AllocationMetadata {
	id<MTLBuffer>	buffer;
	GpuMemory	memory;

	size_t		size;
	void*		cpuAddress;
	void*		gpuAddress;
} Mtl4AllocationMetadata;

typedef struct Mtl4AllocationStorage {
	CmnPage		allocationMetadataPage;
	CmnPage		addressRangeMapPage;

	CmnPool		allocationMetadataPool;
	CmnPool		addressRangeMapNodesPool;

	CmnBTree<Mtl4AddressRange, Mtl4AllocationMetadata*> addressRangeMap;
	// CmnMap<uintptr_t, Mtl4AllocationMetadata*> allocationMap;
} Mtl4AllocationStorage;
extern Mtl4AllocationStorage gMtl4AllocationStorage;

void mtl4PrepareAllocationStorage(GpuResult* result);

void* mtl4Malloc(size_t size, size_t align, GpuMemory memory, GpuResult* result);
void  mtl4Free(void* ptr);
void* mtl4HostToDevicePointer(void* ptr, GpuResult* result);

#endif

