#include "queue.h"

#include <lib/metal4/context.h>

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

	id<MTL4CommandQueue> queue = [gMtl4Context.device newMTL4CommandQueue];
	if (queue == nil) {
		CMN_SET_RESULT(result, GPU_COUND_NOT_CREATE_QUEUE);
		return {};
	}

	{
		CmnScopedWriteRWMutex guard(&gMtl4QueueStorage.mutex);

		Mtl4Queue handle = cmnInsert(&gMtl4QueueStorage.queues, queue, &localResult);
		if (localResult != CMN_SUCCESS) {
			[queue release];

			CMN_SET_RESULT(result, GPU_OUT_OF_CPU_MEMORY);
			return {};
		}

		CMN_SET_RESULT(result, GPU_SUCCESS);
		return mtl4HandleToGpuQueue(handle);
	}
}

GpuCommandBuffer mtl4StartCommandEncoding(GpuQueue queue, GpuResult* result) {
	assert(false && "Unimplemented");
}

void mtl4Submit(GpuQueue queue, GpuCommandBuffer* commandBuffers, size_t commandBufferCount, GpuResult* result) {
	assert(false && "Unimplemented");
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

id<MTL4CommandQueue> mtl4Mtl4QueueOf(Mtl4Queue queue) {
	CmnScopedReadRWMutex guard(&gMtl4QueueStorage.mutex);

	bool wasHandleValid;
	id<MTL4CommandQueue> metalQueue = cmnGet(&gMtl4QueueStorage.queues, queue, &wasHandleValid);
	if (!wasHandleValid) {
		return nil;
	}

	return metalQueue;
}

