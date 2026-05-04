#include "queue.h"

#include <lib/metal4/context.h>
#include <lib/metal4/command_buffers.h>

Mtl4QueueStorage gMtl4QueueStorage;

void mtl4InitQueueStorage(GpuResult* result) {
	CmnResult localResult;

	gMtl4QueueStorage.page = cmnCreatePage(32 * 1024, CMN_PAGE_READABLE | CMN_PAGE_WRITABLE, &localResult);
	if (localResult != CMN_SUCCESS) {
		CMN_SET_RESULT(result, GPU_OUT_OF_CPU_MEMORY);
		return;
	}

	gMtl4QueueStorage.arena = cmnPageToArena(gMtl4QueueStorage.page);
	CmnAllocator allocator = cmnArenaAllocator(&gMtl4QueueStorage.arena);

	cmnCreateHandleMap(&gMtl4QueueStorage.queues, allocator, {}, &localResult);
	if (localResult != CMN_SUCCESS) {
		CMN_SET_RESULT(result, GPU_OUT_OF_CPU_MEMORY);
		return;
	}

	CMN_SET_RESULT(result, GPU_SUCCESS);
}

void mtl4FiniQueueStorage(void) {
	// TODO: Free all queues
	cmnDestroyPage(gMtl4QueueStorage.page);

	gMtl4QueueStorage = {};
}

GpuQueue mtl4CreateQueue(GpuResult* result) {
	CmnResult localResult;
	GpuResult localGpuResult;

	Mtl4QueueMetadata metadata = {};

	metadata.queue = [gMtl4Context.device newMTL4CommandQueue];
	if (metadata.queue == nil) {
		CMN_SET_RESULT(result, GPU_COUND_NOT_CREATE_QUEUE);
		return {};
	}

	mtl4CreateFenceStorage(&metadata.fences, metadata.queue, &localGpuResult);
	if (localGpuResult != GPU_SUCCESS) {
		[metadata.queue release];
		mtl4DestroyFenceStorage(&metadata.fences);

		CMN_SET_RESULT(result, localGpuResult);
		return {};
	}

	{
		CmnScopedWriteRWMutex guard(&gMtl4QueueStorage.mutex);

		Mtl4Queue handle = cmnInsert(&gMtl4QueueStorage.queues, metadata, &localResult);
		if (localResult != CMN_SUCCESS) {
			[metadata.queue release];
			mtl4DestroyFenceStorage(&metadata.fences);

			CMN_SET_RESULT(result, GPU_OUT_OF_CPU_MEMORY);
			return {};
		}

		CMN_SET_RESULT(result, GPU_SUCCESS);
		return mtl4HandleToGpuQueue(handle);
	}
}

id<MTL4CommandQueue> mtl4Mtl4QueueOf(Mtl4Queue queue) {
	CmnScopedReadRWMutex guard(&gMtl4QueueStorage.mutex);

	bool wasHandleValid;
	Mtl4QueueMetadata* metadata = &cmnGet(&gMtl4QueueStorage.queues, queue, &wasHandleValid);
	if (!wasHandleValid) {
		return nil;
	}

	return metadata->queue;
}

Mtl4FenceStorage* mtl4FenceStorageOf(Mtl4Queue queue) {
	CmnScopedReadRWMutex guard(&gMtl4QueueStorage.mutex);

	bool wasHandleValid;
	Mtl4QueueMetadata* metadata = &cmnGet(&gMtl4QueueStorage.queues, queue, &wasHandleValid);
	if (!wasHandleValid) {
		return nil;
	}

	return &metadata->fences;
}

