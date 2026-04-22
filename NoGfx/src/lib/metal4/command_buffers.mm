#include "command_buffers.h"

#include <lib/common/heap_allocator.h>
#include <lib/metal4/context.h>
#include <lib/metal4/allocation.h>

Mtl4CommandBufferStorage gMtl4CommandBufferStorage;

void mtl4InitCommandBufferStorage(GpuResult* result) {
	CmnResult localResult;

	gMtl4CommandBufferStorage.page = cmnCreatePage(32 * 1024, CMN_PAGE_READABLE | CMN_PAGE_WRITABLE, &localResult);
	if (localResult != CMN_SUCCESS) {
		CMN_SET_RESULT(result, GPU_OUT_OF_CPU_MEMORY);
		return;
	}

	gMtl4CommandBufferStorage.arena = cmnPageToArena(gMtl4CommandBufferStorage.page);
	CmnAllocator allocator = cmnArenaAllocator(&gMtl4CommandBufferStorage.arena);

	cmnCreateHandleMap(&gMtl4CommandBufferStorage.commandBuffers, allocator, {}, &localResult);
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

void mtl4Submit(GpuQueue queue, GpuCommandBuffer* commandBuffers, size_t commandBufferCount, GpuResult* result) {
	CmnResult localResult;

	Mtl4Queue queueHandle = mtl4GpuQueueToHandle(queue);
	id<MTL4CommandQueue> metalQueue = mtl4Mtl4QueueOf(queueHandle);
	if (metalQueue == nil) {
		CMN_SET_RESULT(result, GPU_NO_SUCH_QUEUE_FOUND);
	}

	CMN_SET_RESULT(result, GPU_SUCCESS);

	// TODO: Switch thread local arena.
	id<MTL4CommandBuffer>* metalCommandBuffers = cmnHeapAlloc<id<MTL4CommandBuffer>>(commandBufferCount, &localResult);
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

	[metalQueue commit:metalCommandBuffers count:validCommandBufferCount];

	for (size_t i = 0; i < commandBufferCount; i++) {
		GpuCommandBuffer commandBuffer = commandBuffers[i];
		Mtl4CommandBuffer commandBufferHandle = mtl4GpuCommandBufferToHandle(commandBuffer);
		
		// TODO: Schedule the deletion of the submitted command buffers.
	}

	// NOTE: Result here is GPU_SUCCESS if all the command buffers were valid.
	return;
}

void mtl4SubmitWithSignal(
	GpuQueue queue,
	GpuCommandBuffer* commandBuffers,
	size_t commandBufferCount,
	GpuSemaphore semaphore,
	uint64_t value,
	GpuResult* result
) {
	assert(false && "Unimplemented");
}

void gpuMemCpy(GpuCommandBuffer cb, void* destGpu, void* srcGpu, size_t size, GpuResult* result) {
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

void gpuCopyToTexture(GpuCommandBuffer cb, void* destGpu, void* srcGpu, GpuTexture texture, GpuResult* result) {
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

	// [metadata->computeEncoder copyFromTexture:textureMetadata->texture
	// 	 sourceSlice:0 sourceLevel:0 sourceOrigin:(MTLOrigin) sourceSize:(MTLSize) toBuffer:(nonnull id<MTLBuffer>) destinationOffset:(NSUInteger) destinationBytesPerRow:(NSUInteger) destinationBytesPerImage:(NSUInteger)];

	CMN_SET_RESULT(result, GPU_SUCCESS);
}

void gpuCopyFromTexture(GpuCommandBuffer cb, void* destGpu, void* srcGpu, GpuTexture texture, GpuResult* result);

void gpuSetActiveTextureHeapPtr(GpuCommandBuffer cb, void *ptrGpu, GpuResult* result) {
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

Mtl4CommandBuffer mtl4CreateCommandBuffer(GpuResult* result) {
	CmnResult localResult;

	Mtl4CommandBufferMetadata metadata = {};

	metadata.commandBuffer = [gMtl4Context.device newCommandBuffer];
	if (metadata.commandBuffer == nil) {
		CMN_SET_RESULT(result, GPU_COUND_NOT_CREATE_COMMAND_BUFFER);
		return {};
	}

	[metadata.commandBuffer beginCommandBufferWithAllocator:gMtl4CommandBufferStorage.commandAllocator];

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

