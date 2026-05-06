#include "command_buffers.h"

#include <lib/common/heap_allocator.h>
#include <lib/common/atomic.h>
#include <lib/common/futex.h>
#include <lib/metal4/context.h>
#include <lib/metal4/tables.h>
#include <lib/metal4/allocation.h>
#include <lib/metal4/fences.h>
#include <lib/metal4/semaphores.h>

Mtl4CommandBufferStorage gMtl4CommandBufferStorage;

void mtl4InitCommandBufferStorage(GpuResult* result) {
	CmnResult localResult;

	gMtl4CommandBufferStorage.arenaPage = cmnCreatePage(32 * 1024, CMN_PAGE_READABLE | CMN_PAGE_WRITABLE, &localResult);
	if (localResult != CMN_SUCCESS) {
		CMN_SET_RESULT(result, GPU_OUT_OF_CPU_MEMORY);
		return;
	}

	gMtl4CommandBufferStorage.poolPage = cmnCreatePage(4 * 1024, CMN_PAGE_READABLE | CMN_PAGE_WRITABLE, &localResult);
	if (localResult != CMN_SUCCESS) {
		CMN_SET_RESULT(result, GPU_OUT_OF_CPU_MEMORY);
		return;
	}

	gMtl4CommandBufferStorage.arena = cmnPageToArena(gMtl4CommandBufferStorage.arenaPage);
	gMtl4CommandBufferStorage.arenaAllocator = cmnArenaAllocator(&gMtl4CommandBufferStorage.arena);

	gMtl4CommandBufferStorage.pool = cmnPageToPool(gMtl4CommandBufferStorage.poolPage, MTL4_COMMANDBUFFERSTORAGE_POOLSIZE);
	gMtl4CommandBufferStorage.poolAllocator = cmnPoolAllocator(&gMtl4CommandBufferStorage.pool);

	cmnCreateHandleMap(&gMtl4CommandBufferStorage.commandBuffers, gMtl4CommandBufferStorage.arenaAllocator, {}, &localResult);
	if (localResult != CMN_SUCCESS) {
		CMN_SET_RESULT(result, GPU_OUT_OF_CPU_MEMORY);
		return;
	}
	
	gMtl4CommandBufferStorage.commandAllocator = [gMtl4Context.device newCommandAllocator];
	if (gMtl4CommandBufferStorage.commandAllocator == nil) {
		CMN_SET_RESULT(result, GPU_OUT_OF_GPU_MEMORY);
		return;
	}

	CMN_SET_RESULT(result, GPU_SUCCESS);
	return;
}

void mtl4FiniCommandBufferStorage(void) {
	[gMtl4CommandBufferStorage.commandAllocator release];

	gMtl4CommandBufferStorage = {};
	return;
}

GpuCommandBuffer mtl4StartCommandEncoding(GpuQueue queue, GpuResult* result) {
	(void)queue;

	Mtl4CommandBuffer commandBuffer = mtl4CreateCommandBuffer(result);
	return mtl4HandleToGpuCommandBuffer(commandBuffer);
}

void mtl4SubmitRaw(
	GpuQueue queue,
	GpuCommandBuffer* commandBuffers,
	size_t commandBufferCount,
	GpuSemaphore semaphore,
	uint64_t value,
	GpuResult* result
) {
	(void)queue;

	CmnResult localResult;

	CMN_SET_RESULT(result, GPU_SUCCESS);

	id<MTL4CommandQueue> metalQueue = mtl4Queue();

	// TODO: Switch thread local arena.
	id<MTL4CommandBuffer>* metalCommandBuffers = cmnHeapAlloc<id<MTL4CommandBuffer>>(commandBufferCount + 1, &localResult);
	defer (cmnHeapFree(metalCommandBuffers));
	size_t validCommandBufferCount = 0;

	for (size_t i = 0; i < commandBufferCount; i++) {
		GpuCommandBuffer commandBuffer = commandBuffers[i];
		Mtl4CommandBuffer commandBufferHandle = mtl4GpuCommandBufferToHandle(commandBuffer);

		Mtl4CommandBufferMetadata* metadata = mtl4AcquireCommandBufferMetadataFrom(commandBufferHandle);
		if (metadata == nullptr) {
			CMN_SET_RESULT(result, GPU_NO_SUCH_COMMAND_BUFFER_FOUND);
			continue;
		}
		defer (mtl4ReleaseCommandBufferMetadata());

		if (metadata->status != MTL4_COMMAND_BUFFER_ENCODING) {
			CMN_SET_RESULT(result, GPU_USE_AFTER_FREE);
			continue;
		}
		metadata->status = MTL4_COMMAND_BUFFER_SUBMITTED;

		[metadata->computeEncoder endEncoding];
		[metadata->commandBuffer endCommandBuffer];

		metalCommandBuffers[validCommandBufferCount] = metadata->commandBuffer;
		validCommandBufferCount++;
	}

	if (validCommandBufferCount > 0) {
		[metalQueue addResidencySet:gMtl4AllocationStorage.residencySet];
		[metalQueue commit:metalCommandBuffers count:validCommandBufferCount];
	}

	if (semaphore != 0) {
		Mtl4Semaphore semaphoreHandle = mtl4GpuSemaphoreToHandle(semaphore);
		Mtl4SemaphoreMetadata* metadata = mtl4AcquireSemaphoreMetadataFrom(semaphoreHandle);
		if (metadata == nullptr) {
			CMN_SET_RESULT(result, GPU_NO_SUCH_SEMAPHORE_FOUND);
			return;
		}
		defer (mtl4ReleaseSemaphoreMetadata());

		[metalQueue signalEvent:metadata->event value:value];
	}

	// NOTE: Result here is GPU_SUCCESS if all the command buffers were valid.
	return;
}

void mtl4Submit(GpuQueue queue, GpuCommandBuffer* commandBuffers, size_t commandBufferCount, GpuResult* result) {
	mtl4SubmitRaw(queue, commandBuffers, commandBufferCount, 0, 0, result);
}

void mtl4SubmitWithSignal(
	GpuQueue queue,
	GpuCommandBuffer* commandBuffers,
	size_t commandBufferCount,
	GpuSemaphore semaphore,
	uint64_t value,
	GpuResult* result
) {
	mtl4SubmitRaw(queue, commandBuffers, commandBufferCount, semaphore, value, result);
}

void mtl4MemCpy(GpuCommandBuffer cb, void* destGpu, void* srcGpu, size_t size, GpuResult* result) {
	GpuResult localResult;

	Mtl4CommandBuffer handle = mtl4GpuCommandBufferToHandle(cb);
	Mtl4CommandBufferMetadata* metadata = mtl4AcquireCommandBufferMetadataFrom(handle);
	if (metadata == nullptr) {
		CMN_SET_RESULT(result, GPU_NO_SUCH_COMMAND_BUFFER_FOUND);
		return;
	}
	defer (mtl4ReleaseCommandBufferMetadata());

	Mtl4GpuAddress destination = mtl4PtrToGpuAddress(destGpu);
	Mtl4GpuAddress source = mtl4PtrToGpuAddress(srcGpu);

	Mtl4AllocationMetadata* destinationMetadata = mtl4AcquireAllocationMetadataFromGpuPtr(destination);
	if (destinationMetadata == nullptr) {
		CMN_SET_RESULT(result, GPU_NO_SUCH_ALLOCATION_FOUND);
		return;
	}
	defer (mtl4ReleaseAllocationMetadata());

	Mtl4AllocationMetadata* sourceMetadata = mtl4AcquireAllocationMetadataFromGpuPtr(source);
	if (sourceMetadata == nullptr) {
		CMN_SET_RESULT(result, GPU_NO_SUCH_ALLOCATION_FOUND);
		return;
	}
	defer (mtl4ReleaseAllocationMetadata());

	mtl4EnsureBackingBufferIsAllocated(destination, &localResult);
	if (localResult != GPU_SUCCESS) {
		CMN_SET_RESULT(result, localResult);
		return;
	}

	// NOTE: Let's assume that the source buffer is committed. The validation layer will ensure this.
	[metadata->computeEncoder
	 	copyFromBuffer:sourceMetadata->buffer sourceOffset:source.offset
		toBuffer:destinationMetadata->buffer destinationOffset:destination.offset
		size:size];

	CMN_SET_RESULT(result, GPU_SUCCESS);
}

void mtl4CopyToTexture(GpuCommandBuffer cb, void* destGpu, void* srcGpu, GpuTexture texture, GpuResult* result) {
	(void)destGpu;
	
	Mtl4CommandBuffer handle = mtl4GpuCommandBufferToHandle(cb);
	Mtl4CommandBufferMetadata* metadata = mtl4AcquireCommandBufferMetadataFrom(handle);
	if (metadata == nullptr) {
		CMN_SET_RESULT(result, GPU_NO_SUCH_COMMAND_BUFFER_FOUND);
		return;
	}
	defer (mtl4ReleaseCommandBufferMetadata());

	Mtl4GpuAddress source = mtl4PtrToGpuAddress(srcGpu);
	Mtl4AllocationMetadata* sourceMetadata = mtl4AcquireAllocationMetadataFromGpuPtr(source);
	if (sourceMetadata == nullptr) {
		CMN_SET_RESULT(result, GPU_NO_SUCH_ALLOCATION_FOUND);
		return;
	}
	defer (mtl4ReleaseAllocationMetadata());

	Mtl4Texture textureHandle = mtl4GpuTextureToHadle(texture);
	Mtl4TextureMetadata* textureMetadata = mtl4AcquireTextureMetadataFrom(textureHandle);
	if (textureMetadata == nullptr) {
		CMN_SET_RESULT(result, GPU_NO_SUCH_TEXTURE_FOUND);
		return;
	}
	defer (mtl4ReleaseTextureMetadata());

	// TODO: Support arrays.
	if (textureMetadata->descriptor.type == GPU_TEXTURE_2D_ARRAY ||
		textureMetadata->descriptor.type == GPU_TEXTURE_CUBE_ARRAY
	) {
		assert(false && "Unimplemented");
	}

	MTLSize textureSize = MTLSizeMake(
		textureMetadata->descriptor.dimensions[0],
		textureMetadata->descriptor.dimensions[1],
		textureMetadata->descriptor.dimensions[2]
	);
	size_t bytesPerRow = textureMetadata->descriptor.dimensions[0] * gMtl4GpuFormatPixelSize[textureMetadata->descriptor.format];
	size_t bytesPerImage = textureMetadata->descriptor.dimensions[0] *
				textureMetadata->descriptor.dimensions[1] *
				textureMetadata->descriptor.dimensions[2] *
				gMtl4GpuFormatPixelSize[textureMetadata->descriptor.format];

	// TODO: Support mipmaps.
	[metadata->computeEncoder copyFromBuffer:sourceMetadata->buffer
	 	sourceOffset:source.offset
		sourceBytesPerRow:bytesPerRow
		sourceBytesPerImage:bytesPerImage
		sourceSize:textureSize
		toTexture:textureMetadata->texture
		destinationSlice:0
		destinationLevel:0
		destinationOrigin:MTLOriginMake(0, 0, 0)];

	CMN_SET_RESULT(result, GPU_SUCCESS);
}

void mtl4CopyFromTexture(GpuCommandBuffer cb, void* destGpu, void* srcGpu, GpuTexture texture, GpuResult* result) {
	(void)srcGpu;

	Mtl4CommandBuffer handle = mtl4GpuCommandBufferToHandle(cb);
	Mtl4CommandBufferMetadata* metadata = mtl4AcquireCommandBufferMetadataFrom(handle);
	if (metadata == nullptr) {
		CMN_SET_RESULT(result, GPU_NO_SUCH_COMMAND_BUFFER_FOUND);
		return;
	}
	defer (mtl4ReleaseCommandBufferMetadata());

	Mtl4GpuAddress destination = mtl4PtrToGpuAddress(destGpu);
	Mtl4AllocationMetadata* destinationMetadata = mtl4AcquireAllocationMetadataFromGpuPtr(destination);
	if (destinationMetadata == nullptr) {
		CMN_SET_RESULT(result, GPU_NO_SUCH_ALLOCATION_FOUND);
		return;
	}
	defer (mtl4ReleaseAllocationMetadata());

	Mtl4Texture textureHandle = mtl4GpuTextureToHadle(texture);
	Mtl4TextureMetadata* textureMetadata = mtl4AcquireTextureMetadataFrom(textureHandle);
	if (textureMetadata == nullptr) {
		CMN_SET_RESULT(result, GPU_NO_SUCH_TEXTURE_FOUND);
		return;
	}
	defer (mtl4ReleaseTextureMetadata());

	// TODO: Support arrays.
	if (textureMetadata->descriptor.type == GPU_TEXTURE_2D_ARRAY ||
		textureMetadata->descriptor.type == GPU_TEXTURE_CUBE_ARRAY
	) {
		assert(false && "Unimplemented");
	}

	MTLSize textureSize = MTLSizeMake(
		textureMetadata->descriptor.dimensions[0],
		textureMetadata->descriptor.dimensions[1],
		textureMetadata->descriptor.dimensions[2]
	);
	size_t bytesPerRow = textureMetadata->descriptor.dimensions[0] * gMtl4GpuFormatPixelSize[textureMetadata->descriptor.format];
	size_t bytesPerImage = textureMetadata->descriptor.dimensions[0] *
				textureMetadata->descriptor.dimensions[1] *
				textureMetadata->descriptor.dimensions[2] *
				gMtl4GpuFormatPixelSize[textureMetadata->descriptor.format];

	// TODO: Support mipmaps.
	[metadata->computeEncoder copyFromTexture:textureMetadata->texture
		sourceSlice:0
		sourceLevel:0
		sourceOrigin:MTLOriginMake(0, 0, 0)
		sourceSize:textureSize
		toBuffer:destinationMetadata->buffer
		destinationOffset:destination.offset
		destinationBytesPerRow:bytesPerRow
		destinationBytesPerImage:bytesPerImage
	];

}

void mtl4SetActiveTextureHeapPtr(GpuCommandBuffer cb, void *ptrGpu, GpuResult* result) {
	Mtl4CommandBuffer handle = mtl4GpuCommandBufferToHandle(cb);
	Mtl4CommandBufferMetadata* metadata = mtl4AcquireCommandBufferMetadataFrom(handle);
	if (metadata == nullptr) {
		CMN_SET_RESULT(result, GPU_NO_SUCH_COMMAND_BUFFER_FOUND);
		return;
	}
	defer (mtl4ReleaseCommandBufferMetadata());

	CMN_SET_RESULT(result, GPU_SUCCESS);
	metadata->boundTextureHeap = ptrGpu;
}

void mtl4Barrier(GpuCommandBuffer cb, GpuStage before, GpuStage after, GpuHazardFlags hazards, GpuResult* result) {
	Mtl4CommandBuffer handle = mtl4GpuCommandBufferToHandle(cb);
	Mtl4CommandBufferMetadata* metadata = mtl4AcquireCommandBufferMetadataFrom(handle);
	if (metadata == nullptr) {
		CMN_SET_RESULT(result, GPU_NO_SUCH_COMMAND_BUFFER_FOUND);
		return;
	}
	defer (mtl4ReleaseCommandBufferMetadata());

	MTLStages metalBefore = mtl4GpuToMtlStage(before);
	MTLStages metalAfter = mtl4GpuToMtlStage(after);
	MTL4VisibilityOptions metalVisibilityOptions = mtl4GpuHazardsToMtlVisibilityOptions(hazards);

	// TODO: Figure out render stuff...

	if (mtl4IsStageCompute(before)) {
		[metadata->computeEncoder endEncoding];
		metadata->computeEncoder = [metadata->commandBuffer computeCommandEncoder];
	}
	if (mtl4IsStageRender(before)) {
		assert(false && "Unimplemented.");
	}

	if (mtl4IsStageCompute(after)) {
		[metadata->computeEncoder barrierAfterQueueStages:metalBefore beforeStages:metalAfter visibilityOptions:metalVisibilityOptions];
	}
	if (mtl4IsStageRender(after)) {
		assert(false && "Unimplemented.");
	}

	CMN_SET_RESULT(result, GPU_SUCCESS);
}

void mtl4SignalAfter(GpuCommandBuffer cb, GpuStage before, void* ptrGpu, uint64_t value, GpuSignal signal, GpuResult* result) {
	assert(signal == GPU_SIGNAL_ATOMIC_SET && "The only supported signal operation is GPU_SIGNAL_ATOMIC_SET.");

	Mtl4CommandBuffer handle = mtl4GpuCommandBufferToHandle(cb);
	Mtl4CommandBufferMetadata* metadata = mtl4AcquireCommandBufferMetadataFrom(handle);
	if (metadata == nullptr) {
		CMN_SET_RESULT(result, GPU_NO_SUCH_COMMAND_BUFFER_FOUND);
		return;
	}
	defer (mtl4ReleaseCommandBufferMetadata());

	mtl4SignalFence(metadata, before, ptrGpu, value, result);
}

void mtl4WaitBefore(GpuCommandBuffer cb, GpuStage after, void* ptrGpu, uint64_t value, GpuOp op, GpuHazardFlags hazards, uint64_t mask, GpuResult* result) {
	(void)hazards;
	assert(op == GPU_OP_EQUAL && "The only supported wait operation is GPU_OP_EQUAL.");
	assert(mask == ~(uint64_t)0 && "The only supported mask is ~0.");

	Mtl4CommandBuffer handle = mtl4GpuCommandBufferToHandle(cb);
	Mtl4CommandBufferMetadata* metadata = mtl4AcquireCommandBufferMetadataFrom(handle);
	if (metadata == nullptr) {
		CMN_SET_RESULT(result, GPU_NO_SUCH_COMMAND_BUFFER_FOUND);
		return;
	}
	defer (mtl4ReleaseCommandBufferMetadata());

	mtl4WaitFence(metadata, after, ptrGpu, value, result);
}

Mtl4CommandBuffer mtl4CreateCommandBuffer(GpuResult* result) {
	CmnResult localResult;

	Mtl4CommandBufferMetadata metadata = {};

	metadata.commandBuffer = [gMtl4Context.device newCommandBuffer];
	if (metadata.commandBuffer == nil) {
		CMN_SET_RESULT(result, GPU_COUND_NOT_CREATE_COMMAND_BUFFER);
		return {};
	}

	id<MTL4CommandAllocator> commandAllocator = [gMtl4Context.device newCommandAllocator];
	[metadata.commandBuffer beginCommandBufferWithAllocator:commandAllocator];

	metadata.computeEncoder = [metadata.commandBuffer computeCommandEncoder];
	if (metadata.computeEncoder == nil) {
		[metadata.commandBuffer endCommandBuffer];
		[metadata.commandBuffer release];

		CMN_SET_RESULT(result, GPU_COUND_NOT_CREATE_COMMAND_BUFFER);
		return {};
	}

	{
		CmnScopedStorageSyncLockWrite guard(&gMtl4CommandBufferStorage.sync);

		Mtl4CommandBuffer handle = cmnInsert(&gMtl4CommandBufferStorage.commandBuffers, metadata, &localResult);
		if (localResult != CMN_SUCCESS) {
			[metadata.computeEncoder release];
			[metadata.commandBuffer endCommandBuffer];
			[metadata.commandBuffer release];

			CMN_SET_RESULT(result, GPU_OUT_OF_CPU_MEMORY);
			return {};
		}

		CMN_SET_RESULT(result, GPU_SUCCESS);
		return handle;
	}
}

void mtl4DestroyCommandBuffer(Mtl4CommandBuffer commandBuffer) {
	bool wasHandleValid;
	Mtl4CommandBufferMetadata* metadata = &cmnGet(&gMtl4CommandBufferStorage.commandBuffers, commandBuffer, &wasHandleValid);
	if (!wasHandleValid) {
		return;
	}

	if (metadata->status != MTL4_COMMAND_BUFFER_SUBMITTED) {
		return;
	}

	[metadata->commandBuffer release];

	cmnRemove(&gMtl4CommandBufferStorage.commandBuffers, commandBuffer);
}

bool mtl4IsCommandBufferScheduledForDeletion(Mtl4CommandBuffer commandBuffer) {
	Mtl4CommandBufferMetadata* metadata = mtl4AcquireCommandBufferMetadataFrom(commandBuffer);
	if (metadata == nil) {
		return false;
	}

	return metadata->status == MTL4_COMMAND_BUFFER_SUBMITTED;
}

bool mtl4IsStageCompute(GpuStage stage) {
	return GPU_STAGE_COMPUTE & stage || GPU_STAGE_TRANSFER & stage;
}

bool mtl4IsStageRender(GpuStage stage) {
	return GPU_STAGE_PIXEL_SHADER & stage || GPU_STAGE_RASTER_COLOR_OUT & stage || GPU_STAGE_VERTEX_SHADER & stage;
}

bool mtl4CanImposeNormalMtlBarrierBetween(GpuStage before, GpuStage after, GpuHazardFlags hazards) {
	(void)hazards;

	bool cannotImpose = before & GPU_STAGE_PIXEL_SHADER ||
		before & GPU_STAGE_RASTER_COLOR_OUT ||
		after & GPU_STAGE_PIXEL_SHADER ||
		after & GPU_STAGE_RASTER_COLOR_OUT;
	return !cannotImpose;
}

MTLStages mtl4GpuToMtlStage(GpuStage stage) {
	MTLStages stages = 0;

	if (stage & GPU_STAGE_TRANSFER) {
		stages |= MTLStageBlit;
	}
	if (stage & GPU_STAGE_COMPUTE) {
		stages |= MTLStageDispatch;
	}
	if (stage & GPU_STAGE_VERTEX_SHADER) {
		stages |= MTLStageVertex;
	}
	if (stage & GPU_STAGE_PIXEL_SHADER) {
		stages |= MTLStageFragment;
	}
	if (stage & GPU_STAGE_RASTER_COLOR_OUT) {
		stages |= MTLStageTile;
	}

	return stages;
}

MTL4VisibilityOptions mtl4GpuHazardsToMtlVisibilityOptions(GpuHazardFlags hazards) {
	MTL4VisibilityOptions options = MTL4VisibilityOptionNone;

	if (hazards & GPU_HAZARD_DESCRIPTORS) {
		options |= MTL4VisibilityOptionResourceAlias;
	}
	if (hazards & GPU_HAZARD_DRAW_ARGUMENTS) {
		options |= MTL4VisibilityOptionResourceAlias;
	}
	if (hazards & GPU_HAZARD_DEPTH_STENCIL) {
		options |= MTL4VisibilityOptionDevice;
	}

	return options;
}

Mtl4CommandBufferMetadata* mtl4AcquireCommandBufferMetadataFrom(Mtl4CommandBuffer handle) {
	bool wasHandleValid;
	Mtl4CommandBufferMetadata* metadata = cmnStorageSyncAcquireResource(
		&gMtl4CommandBufferStorage.commandBuffers,
		&gMtl4CommandBufferStorage.sync,
		handle,
		&wasHandleValid
	);
	if (!wasHandleValid) {
		return nullptr;
	}

	return metadata;
}

void mtl4ReleaseCommandBufferMetadata(void) {
	cmnStorageSyncReleaseResource(&gMtl4CommandBufferStorage.sync);
}

