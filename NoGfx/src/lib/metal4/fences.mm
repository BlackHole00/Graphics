#include "fences.h"

#include <lib/common/heap_allocator.h>
#include <lib/metal4/command_buffers.h>
#include <lib/metal4/context.h>
#include <lib/metal4/allocation.h>

Mtl4FenceStorage gMtl4FenceStorage;

void mtl4InitFenceStorage(GpuResult* result) {
	CmnResult localResult;

	gMtl4FenceStorage = {};

	gMtl4FenceStorage.page = cmnCreatePage(1024 * 1024, CMN_PAGE_READABLE | CMN_PAGE_WRITABLE, &localResult);
	if (localResult != CMN_SUCCESS) {
		CMN_SET_RESULT(result, GPU_OUT_OF_CPU_MEMORY);
		return;
	}

	gMtl4FenceStorage.arena = cmnPageToArena(gMtl4FenceStorage.page);

	cmnCreateHandleMap(&gMtl4FenceStorage.fences, cmnArenaAllocator(&gMtl4FenceStorage.arena), {}, nullptr);

	cmnCreateHashMap(&gMtl4FenceStorage.lookup, 1024, {}, cmnHeapAllocator(), &localResult);
	if (localResult != CMN_SUCCESS) {
		CMN_SET_RESULT(result, GPU_OUT_OF_CPU_MEMORY);
		return;
	}

	gMtl4FenceStorage.fenceUploadBuffer = [gMtl4Context.device newBufferWithLength:1024*1024 options:MTLResourceStorageModeShared | MTLResourceCPUCacheModeDefaultCache];
	gMtl4FenceStorage.fenceUploadBufferSize = 1024 * 1024;
	gMtl4FenceStorage.fenceUploadBufferUsed = 0;

	if (gMtl4FenceStorage.fenceUploadBuffer == nil) {
		CMN_SET_RESULT(result, GPU_OUT_OF_GPU_MEMORY);
		return;
	}

	CMN_SET_RESULT(result, GPU_SUCCESS);
	return;
}

void mtl4FiniFenceStorage() {
	cmnDestroyPage(gMtl4FenceStorage.page);
	[gMtl4FenceStorage.fenceUploadBuffer release];

	gMtl4FenceStorage = {};
}

Mtl4FenceHandle mtl4FenceHandleFrom(void* gpuPtr, uint64_t value, bool* didFindFence) {
	CmnScopedStorageSyncLockRead guard(&gMtl4FenceStorage.sync);
	
	Mtl4FenceId fenceId = { gpuPtr, value };
	return cmnGet(&gMtl4FenceStorage.lookup, fenceId, didFindFence);
}

Mtl4FenceMetadata* mtl4AcquireFenceMetadataFrom(void* gpuPtr, uint64_t value) {
	bool didFindFence;
	Mtl4FenceHandle handle = mtl4FenceHandleFrom(gpuPtr, value, &didFindFence);
	if (!didFindFence) {
		return nullptr;
	}

	Mtl4FenceMetadata* metadata = cmnStorageSyncAcquireResource(&gMtl4FenceStorage.fences, &gMtl4FenceStorage.sync, handle, &didFindFence);
	if (!didFindFence) {
		return nullptr;
	}

	return metadata;
}

Mtl4FenceMetadata* mtl4AcquireOrCreateFenceMetadataFor(void* gpuPtr, uint64_t value, GpuResult* result) {
	CmnResult localResult;

	Mtl4FenceMetadata* metadata = mtl4AcquireFenceMetadataFrom(gpuPtr, value);
	if (metadata != nil) {
		CMN_SET_RESULT(result, GPU_SUCCESS);
		return metadata;
	}

	Mtl4FenceMetadata newMetadata;

	// newMetadata.gpuPtrUpdatedFence = [gMtl4Context.device newFence];
	// if (newMetadata.gpuPtrUpdatedFence == nil) {
	// 	CMN_SET_RESULT(result, GPU_OUT_OF_GPU_MEMORY);
	// 	return metadata;
	// }

	newMetadata.computeWriteGpuPtrFence = [gMtl4Context.device newFence];
	if (newMetadata.computeWriteGpuPtrFence == nil) {
		// [newMetadata.gpuPtrUpdatedFence release];

		CMN_SET_RESULT(result, GPU_OUT_OF_GPU_MEMORY);
		return metadata;
	}

	// newMetadata.renderWriteGpuPtrFence = [gMtl4Context.device newFence];
	// if (newMetadata.renderWriteGpuPtrFence == nil) {
	// 	[newMetadata.computeWriteGpuPtrFence release];
	// 	[newMetadata.gpuPtrUpdatedFence release];

	// 	CMN_SET_RESULT(result, GPU_OUT_OF_GPU_MEMORY);
	// 	return metadata;
	// }

	CmnScopedStorageSyncLockWrite guard(&gMtl4FenceStorage.sync);

	Mtl4FenceId fenceId = { gpuPtr, value };

	bool containsFence;
	Mtl4FenceHandle fenceHandle = cmnGet(&gMtl4FenceStorage.lookup, fenceId, &containsFence);
	if (containsFence) {
		// NOTE: Some other thread beated us on time.

		metadata = &cmnGet(&gMtl4FenceStorage.fences, fenceHandle, &containsFence);
		assert(containsFence && "Something is horrendously wrong here.");

		cmnStorageSyncMarkAsUsingResources(&gMtl4FenceStorage.sync);

		// [newMetadata.gpuPtrUpdatedFence release];
		[newMetadata.computeWriteGpuPtrFence release];
		// [newMetadata.renderWriteGpuPtrFence release];

		CMN_SET_RESULT(result, GPU_SUCCESS);
		return metadata;
	} else {
		fenceHandle = cmnInsert(&gMtl4FenceStorage.fences, newMetadata, &localResult);
		if (localResult != CMN_SUCCESS) {
			// [newMetadata.gpuPtrUpdatedFence release];
			[newMetadata.computeWriteGpuPtrFence release];
			// [newMetadata.renderWriteGpuPtrFence release];

			CMN_SET_RESULT(result, GPU_OUT_OF_CPU_MEMORY);
			return metadata;
		}

		cmnInsert(&gMtl4FenceStorage.lookup, fenceId, fenceHandle, &localResult);
		if (localResult != CMN_SUCCESS) {
			cmnRemove(&gMtl4FenceStorage.fences, fenceHandle);

			// [newMetadata.gpuPtrUpdatedFence release];
			[newMetadata.computeWriteGpuPtrFence release];
			// [newMetadata.renderWriteGpuPtrFence release];

			CMN_SET_RESULT(result, GPU_OUT_OF_CPU_MEMORY);
			return metadata;
		}

		metadata = &cmnGet(&gMtl4FenceStorage.fences, fenceHandle, &containsFence);
		assert(containsFence && "Something is horrendously wrong here.");

		cmnStorageSyncMarkAsUsingResources(&gMtl4FenceStorage.sync);

		CMN_SET_RESULT(result, GPU_SUCCESS)
		return metadata;
	}
}

void mtl4ReleaseFenceMetadata(void) {
	cmnStorageSyncReleaseResource(&gMtl4FenceStorage.sync);
}

size_t mtl4UploadFenceValue(uint64_t value) {
	uintptr_t values = (uintptr_t)[gMtl4FenceStorage.fenceUploadBuffer contents];

	size_t valueOffset;
	for (;;) {
		valueOffset = cmnAtomicLoad(&gMtl4FenceStorage.fenceUploadBufferUsed);
		if (valueOffset >= gMtl4FenceStorage.fenceUploadBufferSize) {
			valueOffset = 0;
		}

		if (cmnAtomicCompareExchangeStrong<uint64_t>(
			&gMtl4FenceStorage.fenceUploadBufferUsed,
			valueOffset,
			valueOffset + sizeof(uint64_t)
		)) {
			break;
		}
	}

	uint64_t* valuePtr = (uint64_t*)(values + valueOffset);
	*valuePtr = value;

	return valueOffset;
}

void mtl4SignalFence(
	Mtl4CommandBufferMetadata* commandBuffer,
	GpuStage before,
	void* gpuPtr,
	uint64_t value,
	GpuResult* result
) {
	GpuResult localResult;

	Mtl4FenceMetadata* metadata = mtl4AcquireOrCreateFenceMetadataFor(gpuPtr, value, &localResult);
	if (localResult != GPU_SUCCESS) {
		CMN_SET_RESULT(result, localResult);
		return;
	}
	defer (mtl4ReleaseFenceMetadata());

	Mtl4AllocationMetadata* allocation = mtl4AcquireAllocationMetadataFromGpuPtr(gpuPtr);
	if (allocation == nullptr) {
		CMN_SET_RESULT(result, GPU_NO_SUCH_ALLOCATION_FOUND);
		return;
	}
	defer (mtl4ReleaseAllocationMetadata());

	mtl4EnsureBackingBufferIsAllocated(allocation, &localResult);
	if (localResult != GPU_SUCCESS) {
		CMN_SET_RESULT(result, localResult);
		return;
	}

	uintptr_t gpuPtrOffsetFromBase = mtl4GpuAddressOffsetFromBase(gpuPtr);

	MTLStages mtlStages	= mtl4GpuToMtlStage(before);

	size_t fenceUploadValueOffset = mtl4UploadFenceValue(value);

	if ([commandBuffer->computeEncoder stages] != 0) {
		[commandBuffer->computeEncoder endEncoding];
		commandBuffer->computeEncoder = [commandBuffer->commandBuffer computeCommandEncoder];
	}

	[commandBuffer->computeEncoder barrierAfterQueueStages:mtlStages beforeStages:MTLStageBlit visibilityOptions:MTL4VisibilityOptionNone];
	[commandBuffer->computeEncoder
		copyFromBuffer:gMtl4FenceStorage.fenceUploadBuffer
		sourceOffset:fenceUploadValueOffset
		toBuffer:allocation->buffer
		destinationOffset:gpuPtrOffsetFromBase
		size:sizeof(uint64_t)];
	[commandBuffer->computeEncoder updateFence:metadata->computeWriteGpuPtrFence afterEncoderStages:MTLStageBlit];
	[commandBuffer->computeEncoder endEncoding];

	commandBuffer->computeEncoder = [commandBuffer->commandBuffer computeCommandEncoder];
	[commandBuffer->computeEncoder waitForFence:metadata->computeWriteGpuPtrFence beforeEncoderStages:MTLStageBlit];
	// [commandBuffer->computeEncoder
	//  	barrierAfterQueueStages:MTLStageBlit
	//  	beforeStages:MTLStageBlit | MTLStageDispatch | MTLStageAccelerationStructure
	//  	visibilityOptions:MTL4VisibilityOptionDevice | MTL4VisibilityOptionResourceAlias];
}

void mtl4WaitFence(
	Mtl4CommandBufferMetadata* commandBuffer,
	GpuStage after,
	void* gpuPtr,
	uint64_t value,
	GpuResult* result
) {
	GpuResult localResult;

	Mtl4FenceMetadata* metadata = mtl4AcquireOrCreateFenceMetadataFor(gpuPtr, value, &localResult);
	if (localResult != GPU_SUCCESS) {
		CMN_SET_RESULT(result, localResult);
		return;
	}
	defer (mtl4ReleaseFenceMetadata());

	MTLStages mtlComputeStages = mtl4GpuToMtlStage(after) & (MTLStageBlit | MTLStageDispatch);
	// MTLStages mtlRenderStages = mtl4GpuToMtlStage(after) & (MTLStageTile | MTLStageFragment | MTLStageVertex);

	if (mtl4IsStageCompute(after)) {
		if ([commandBuffer->computeEncoder stages] != 0) {
			[commandBuffer->computeEncoder endEncoding];
			commandBuffer->computeEncoder = [commandBuffer->commandBuffer computeCommandEncoder];
		}

		[commandBuffer->computeEncoder waitForFence:metadata->computeWriteGpuPtrFence beforeEncoderStages:mtlComputeStages];
	}
	// if (mtl4IsStageRender(after)) {
	// 	[commandBuffer->renderEncoder waitForFence:metadata->gpuPtrUpdatedFence beforeEncoderStages:mtlRenderStages];
	// }

	CMN_SET_RESULT(result, GPU_SUCCESS);
}

