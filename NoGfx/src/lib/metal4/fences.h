#ifndef MTL4_FENCES_H
#define MTL4_FENCES_H

#include <gpu/gpu.h>

#include <lib/common/page.h>
#include <lib/common/hash_map.h>
#include <lib/common/handle_map.h>
#include <lib/common/storage_sync.h>

#include <Metal/Metal.h>

struct Mtl4CommandBufferMetadata;

typedef CmnHandle Mtl4FenceHandle;

typedef struct Mtl4FenceMetadata {
	// Signaled after
	id<MTLFence>	gpuPtrUpdatedFence;
	id<MTLFence>	computeWriteGpuPtrFence;
	id<MTLFence>	renderWriteGpuPtrFence;
} Mtl4FenceMetadata;

typedef struct Mtl4FenceId {
	void*		gpuPtr;
	uint64_t 	value;
} Mtl4FenceId;

typedef struct Mtl4FenceStorage {
	CmnPage		page;
	CmnArena	arena;

	id<MTL4CommandAllocator>	allocator;
	id<MTL4CommandQueue>		referenceCommandQueue;
	id<MTL4CommandBuffer>		gpuPtrUpdateCommands;
	id<MTL4ComputeCommandEncoder>	gpuPtrUpdateEncoder;

	id<MTLBuffer>			fenceUploadBuffer;
	uint64_t			fenceUploadBufferSize;
	uint64_t			fenceUploadBufferUsed;

	CmnHashMap	<Mtl4FenceId, Mtl4FenceHandle>	lookup;

	CmnHandleMap	<Mtl4FenceMetadata>	fences;
	CmnStorageSync	sync;
} Mtl4FenceStorage;

void mtl4CreateFenceStorage(Mtl4FenceStorage* storage, id<MTL4CommandQueue> queue, GpuResult* result);
void mtl4DestroyFenceStorage(Mtl4FenceStorage* storage);

Mtl4FenceHandle mtl4FenceHandleFrom(Mtl4FenceStorage* storage, void* gpuPtr, uint64_t value, bool* didFindFence);

Mtl4FenceMetadata* mtl4AcquireFenceMetadataFrom(Mtl4FenceStorage* storage, void* gpuPtr, uint64_t value);
Mtl4FenceMetadata* mtl4AcquireOrCreateFenceMetadataFor(Mtl4FenceStorage* storage, void* gpuPtr, uint64_t value, GpuResult* result);
void mtl4ReleaseFenceMetadata(Mtl4FenceStorage* storage);

void mtl4SignalFence(
	Mtl4FenceStorage* storage,
	Mtl4CommandBufferMetadata* commandBuffer,
	GpuStage after,
	void* gpuPtr,
	uint64_t value,
	GpuResult* result
);
void mtl4WaitFence(
	Mtl4FenceStorage* storage,
	Mtl4CommandBufferMetadata* commandBuffer,
	GpuStage before,
	void* gpuPtr,
	uint64_t value,
	GpuResult* result
);

id<MTL4CommandBuffer> mtl4GetGpuUpdatesCommands(Mtl4FenceStorage* storage, GpuResult* result);

template <>
struct CmnTypeTraits<Mtl4FenceId> {
	static bool eq(const Mtl4FenceId& left, const Mtl4FenceId& right) {
		return left.gpuPtr == right.gpuPtr && left.value == right.value;
	}

	static CmnCmp cmp(const Mtl4FenceId& left, const Mtl4FenceId& right) {
		(void)left; (void)right;
		assert(false && "Unimplemented.");
	}

	static size_t hash(const Mtl4FenceId& value) {
		return cmnHashInteger64((uintptr_t)value.gpuPtr) ^ cmnHashInteger64(value.value);
	}
};


#endif // MTL4_FENCES_H

