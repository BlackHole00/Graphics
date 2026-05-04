#include "fences.h"

#include <lib/common/heap_allocator.h>
#include <lib/metal4/command_buffers.h>
#include <lib/metal4/context.h>
#include <lib/metal4/allocation.h>

void mtl4CreateFenceStorage(Mtl4FenceStorage* storage, id<MTL4CommandQueue> queue, GpuResult* result) {
	CmnResult localResult;

	*storage = {};

	storage->page = cmnCreatePage(1024 * 1024, CMN_PAGE_READABLE | CMN_PAGE_WRITABLE, &localResult);
	if (localResult != CMN_SUCCESS) {
		CMN_SET_RESULT(result, GPU_OUT_OF_CPU_MEMORY);
		return;
	}

	storage->arena = cmnPageToArena(storage->page);

	cmnCreateHandleMap(&storage->fences, cmnArenaAllocator(&storage->arena), {}, nullptr);

	cmnCreateHashMap(&storage->lookup, 1024, {}, cmnHeapAllocator(), &localResult);
	if (localResult != CMN_SUCCESS) {
		CMN_SET_RESULT(result, GPU_OUT_OF_CPU_MEMORY);
		return;
	}

	storage->referenceCommandQueue = queue;

	storage->allocator = [gMtl4Context.device newCommandAllocator];
	if (storage->allocator == nil) {
		CMN_SET_RESULT(result, GPU_OUT_OF_GPU_MEMORY);
		return;
	}

	storage->gpuPtrUpdateCommands = [gMtl4Context.device newCommandBuffer];
	if (storage->gpuPtrUpdateCommands == nil) {
		CMN_SET_RESULT(result, GPU_OUT_OF_GPU_MEMORY);
		return;
	}

	[storage->gpuPtrUpdateCommands beginCommandBufferWithAllocator:storage->allocator];

	storage->gpuPtrUpdateEncoder = [storage->gpuPtrUpdateCommands computeCommandEncoder];
	if (storage->gpuPtrUpdateCommands == nil) {
		CMN_SET_RESULT(result, GPU_OUT_OF_GPU_MEMORY);
		return;
	}

	storage->fenceUploadBuffer = [gMtl4Context.device
		newBufferWithLength:1024 * 1024
		options:MTLResourceStorageModeShared | MTLResourceCPUCacheModeDefaultCache];
	storage->fenceUploadBufferSize = 1024 * 1024;
	storage->fenceUploadBufferUsed = 0;
	memset(storage->fenceUploadBuffer.contents, 0xF0, 1024 * 1024);

	CMN_SET_RESULT(result, GPU_SUCCESS);
	return;
}

void mtl4DestroyFenceStorage(Mtl4FenceStorage* storage) {
	cmnDestroyPage(storage->page);

	[storage->gpuPtrUpdateCommands release];
	[storage->allocator release];

	*storage = {};
}

Mtl4FenceHandle mtl4FenceHandleFrom(Mtl4FenceStorage* storage, void* gpuPtr, uint64_t value, bool* didFindFence) {
	CmnScopedStorageSyncLockRead guard(&storage->sync);
	
	Mtl4FenceId fenceId = { gpuPtr, value };
	return cmnGet(&storage->lookup, fenceId, didFindFence);
}

Mtl4FenceMetadata* mtl4AcquireFenceMetadataFrom(Mtl4FenceStorage* storage, void* gpuPtr, uint64_t value) {
	bool didFindFence;
	Mtl4FenceHandle handle = mtl4FenceHandleFrom(storage, gpuPtr, value, &didFindFence);
	if (!didFindFence) {
		return nullptr;
	}

	Mtl4FenceMetadata* metadata = cmnStorageSyncAcquireResource(&storage->fences, &storage->sync, handle, &didFindFence);
	if (!didFindFence) {
		return nullptr;
	}

	return metadata;
}

Mtl4FenceMetadata* mtl4AcquireOrCreateFenceMetadataFor(Mtl4FenceStorage* storage, void* gpuPtr, uint64_t value, GpuResult* result) {
	CmnResult localResult;

	Mtl4FenceMetadata* metadata = mtl4AcquireFenceMetadataFrom(storage, gpuPtr, value);
	if (metadata != nil) {
		CMN_SET_RESULT(result, GPU_SUCCESS);
		return metadata;
	}

	Mtl4FenceMetadata newMetadata;

	newMetadata.gpuPtrUpdatedFence = [gMtl4Context.device newFence];
	if (newMetadata.gpuPtrUpdatedFence == nil) {
		CMN_SET_RESULT(result, GPU_OUT_OF_GPU_MEMORY);
		return metadata;
	}

	newMetadata.computeWriteGpuPtrFence = [gMtl4Context.device newFence];
	if (newMetadata.computeWriteGpuPtrFence == nil) {
		[newMetadata.gpuPtrUpdatedFence release];

		CMN_SET_RESULT(result, GPU_OUT_OF_GPU_MEMORY);
		return metadata;
	}

	newMetadata.renderWriteGpuPtrFence = [gMtl4Context.device newFence];
	if (newMetadata.renderWriteGpuPtrFence == nil) {
		[newMetadata.computeWriteGpuPtrFence release];
		[newMetadata.gpuPtrUpdatedFence release];

		CMN_SET_RESULT(result, GPU_OUT_OF_GPU_MEMORY);
		return metadata;
	}

	CmnScopedStorageSyncLockWrite guard(&storage->sync);

	Mtl4FenceId fenceId = { gpuPtr, value };

	bool containsFence;
	Mtl4FenceHandle fenceHandle = cmnGet(&storage->lookup, fenceId, &containsFence);
	if (containsFence) {
		// NOTE: Some other thread beated us on time.

		metadata = &cmnGet(&storage->fences, fenceHandle, &containsFence);
		assert(containsFence && "Something is horrendously wrong here.");

		cmnStorageSyncMarkAsUsingResources(&storage->sync);

		[newMetadata.gpuPtrUpdatedFence release];
		[newMetadata.computeWriteGpuPtrFence release];
		[newMetadata.renderWriteGpuPtrFence release];

		CMN_SET_RESULT(result, GPU_SUCCESS);
		return metadata;
	} else {
		fenceHandle = cmnInsert(&storage->fences, newMetadata, &localResult);
		if (localResult != CMN_SUCCESS) {
			[newMetadata.gpuPtrUpdatedFence release];
			[newMetadata.computeWriteGpuPtrFence release];
			[newMetadata.renderWriteGpuPtrFence release];

			CMN_SET_RESULT(result, GPU_OUT_OF_CPU_MEMORY);
			return metadata;
		}

		cmnInsert(&storage->lookup, fenceId, fenceHandle, &localResult);
		if (localResult != CMN_SUCCESS) {
			cmnRemove(&storage->fences, fenceHandle);

			[newMetadata.gpuPtrUpdatedFence release];
			[newMetadata.computeWriteGpuPtrFence release];
			[newMetadata.renderWriteGpuPtrFence release];

			CMN_SET_RESULT(result, GPU_OUT_OF_CPU_MEMORY);
			return metadata;
		}

		metadata = &cmnGet(&storage->fences, fenceHandle, &containsFence);
		assert(containsFence && "Something is horrendously wrong here.");

		cmnStorageSyncMarkAsUsingResources(&storage->sync);

		CMN_SET_RESULT(result, GPU_SUCCESS)
		return metadata;
	}
}

void mtl4ReleaseFenceMetadata(Mtl4FenceStorage* storage) {
	cmnStorageSyncReleaseResource(&storage->sync);
}

void mtl4SignalFence(
	Mtl4FenceStorage* storage,
	Mtl4CommandBufferMetadata* commandBuffer,
	GpuStage after,
	void* gpuPtr,
	uint64_t value,
	GpuResult* result
) {
	GpuResult localResult;

	Mtl4FenceMetadata* metadata = mtl4AcquireOrCreateFenceMetadataFor(storage, gpuPtr, value, &localResult);
	if (localResult != GPU_SUCCESS) {
		CMN_SET_RESULT(result, localResult);
		return;
	}
	defer (mtl4ReleaseFenceMetadata(storage));

	Mtl4AllocationMetadata* allocation = mtl4AcquireAllocationMetadataFromGpuPtr(gpuPtr);
	if (allocation == nullptr || !(allocation->internalUsage & MTL4_ALLOCATION_COMMITTED)) {
		CMN_SET_RESULT(result, GPU_NO_SUCH_ALLOCATION_FOUND);
		return;
	}
	defer (mtl4ReleaseAllocationMetadata());

	MTLStages mtlComputeStages = mtl4GpuToMtlStage(after) & (MTLStageBlit | MTLStageDispatch);
	MTLStages mtlRenderStages = mtl4GpuToMtlStage(after) & (MTLStageTile | MTLStageFragment | MTLStageVertex);

	if (mtl4IsStageCompute(after)) {
		[commandBuffer->computeEncoder updateFence:metadata->computeWriteGpuPtrFence afterEncoderStages:mtlComputeStages];
		[storage->gpuPtrUpdateEncoder waitForFence:metadata->computeWriteGpuPtrFence beforeEncoderStages:MTLStageBlit];
	}
	if (mtl4IsStageRender(after)) {
		[commandBuffer->renderEncoder updateFence:metadata->renderWriteGpuPtrFence afterEncoderStages:mtlRenderStages];
		[storage->gpuPtrUpdateEncoder waitForFence:metadata->renderWriteGpuPtrFence beforeEncoderStages:MTLStageBlit];
	}

	// Use current offset before incrementing (ring-buffer allocation).
	size_t uploadBufferOffset = storage->fenceUploadBufferUsed;
	storage->fenceUploadBufferUsed += sizeof(uint64_t);
	if (storage->fenceUploadBufferUsed >= storage->fenceUploadBufferSize) {
		storage->fenceUploadBufferUsed = 0;
	}
		
	size_t offsetFromBase = mtl4GpuAddressOffsetFromBase(gpuPtr);

	// *(uint64_t*)((uintptr_t)[storage->fenceUploadBuffer contents] + uploadBufferOffset) = value;

	// [storage->gpuPtrUpdateEncoder
	// 	copyFromBuffer:storage->fenceUploadBuffer
	// 	sourceOffset:uploadBufferOffset
	// 	toBuffer:allocation->buffer
	// 	destinationOffset:offsetFromBase
	// 	size:sizeof(uint64_t)];
	[storage->gpuPtrUpdateEncoder
	 	barrierAfterEncoderStages:MTLStageDispatch | MTLStageBlit | MTLStageAccelerationStructure
	 	beforeEncoderStages:MTLStageDispatch | MTLStageBlit | MTLStageAccelerationStructure
	 	visibilityOptions:MTL4VisibilityOptionDevice | MTL4VisibilityOptionResourceAlias];
	[storage->gpuPtrUpdateEncoder
		copyFromBuffer:storage->fenceUploadBuffer
		sourceOffset:0
		toBuffer:allocation->buffer
		destinationOffset:0
		size:allocation->buffer.allocatedSize];
	[storage->gpuPtrUpdateEncoder
	 	barrierAfterEncoderStages:MTLStageDispatch | MTLStageBlit | MTLStageAccelerationStructure
	 	beforeEncoderStages:MTLStageDispatch | MTLStageBlit | MTLStageAccelerationStructure
	 	visibilityOptions:MTL4VisibilityOptionDevice | MTL4VisibilityOptionResourceAlias];
	[storage->gpuPtrUpdateEncoder updateFence:metadata->gpuPtrUpdatedFence afterEncoderStages:MTLStageBlit];
}

void mtl4WaitFence(
	Mtl4FenceStorage* storage,
	Mtl4CommandBufferMetadata* commandBuffer,
	GpuStage before,
	void* gpuPtr,
	uint64_t value,
	GpuResult* result
) {
	GpuResult localResult;

	Mtl4FenceMetadata* metadata = mtl4AcquireOrCreateFenceMetadataFor(storage, gpuPtr, value, &localResult);
	if (localResult != GPU_SUCCESS) {
		CMN_SET_RESULT(result, localResult);
		return;
	}
	defer (mtl4ReleaseFenceMetadata(storage));

	MTLStages mtlComputeStages = mtl4GpuToMtlStage(before) & (MTLStageBlit | MTLStageDispatch);
	MTLStages mtlRenderStages = mtl4GpuToMtlStage(before) & (MTLStageTile | MTLStageFragment | MTLStageVertex);

	if (mtl4IsStageCompute(before)) {
		[commandBuffer->computeEncoder waitForFence:metadata->gpuPtrUpdatedFence beforeEncoderStages:mtlComputeStages];
	}
	if (mtl4IsStageRender(before)) {
		[commandBuffer->renderEncoder waitForFence:metadata->gpuPtrUpdatedFence beforeEncoderStages:mtlRenderStages];
	}

	CMN_SET_RESULT(result, GPU_SUCCESS);
}

id<MTL4CommandBuffer> mtl4GetGpuUpdatesCommands(Mtl4FenceStorage* storage, GpuResult* result) {
	[storage->gpuPtrUpdateEncoder endEncoding];
	[storage->gpuPtrUpdateCommands endCommandBuffer];

	id<MTL4CommandBuffer> commandBuffer = storage->gpuPtrUpdateCommands;

	storage->gpuPtrUpdateCommands = [gMtl4Context.device newCommandBuffer];
	if (storage->gpuPtrUpdateCommands == nil) {
		CMN_SET_RESULT(result, GPU_OUT_OF_GPU_MEMORY);
		return commandBuffer;
	}

	[storage->gpuPtrUpdateCommands beginCommandBufferWithAllocator:storage->allocator];

	storage->gpuPtrUpdateEncoder = [storage->gpuPtrUpdateCommands computeCommandEncoder];
	if (storage->gpuPtrUpdateCommands == nil) {
		CMN_SET_RESULT(result, GPU_OUT_OF_GPU_MEMORY);
		return commandBuffer;
	}

	CMN_SET_RESULT(result, GPU_SUCCESS);
	return commandBuffer;
}

