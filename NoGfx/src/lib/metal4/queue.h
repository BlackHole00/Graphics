#ifndef MTL4_QUEUE_H
#define MTL4_QUEUE_H

#include <gpu/gpu.h>

#include <lib/common/page.h>
#include <lib/common/handle_map.h>
#include <lib/common/rw_mutex.h>

#include <Metal/Metal.h>

typedef CmnHandle Mtl4Queue;

typedef struct Mtl4QueueStorage {
	CmnPage		page;
	CmnArena	arena;

	// NOTE: Mtl4Queues are 1:1 matching with MTL4CommandQueues.
	CmnHandleMap	<id<MTL4CommandQueue>>	queues;
	CmnRWMutex	mutex;
} Mtl4QueueStorage;
extern Mtl4QueueStorage gMtl4QueueStorage;

void mtl4InitQueueStorage(GpuResult* result);
void mtl4FiniQueueStorage(void);

GpuQueue mtl4CreateQueue(GpuResult* result);
GpuCommandBuffer mtl4StartCommandEncoding(GpuQueue queue, GpuResult* result);
void mtl4Submit(GpuQueue queue, GpuCommandBuffer* commandBuffers, size_t commandBufferCount, GpuResult* result);
void mtl4SubmitWithSignal(
	GpuQueue queue,
	GpuCommandBuffer* commandBuffers,
	size_t commandBufferCount,
	GpuSemaphore semaphore,
	uint64_t value,
	GpuResult* result
);

id<MTL4CommandQueue> mtl4Mtl4QueueOf(Mtl4Queue queue);

inline Mtl4Queue mtl4GpuQueueToHandle(GpuQueue queue) {
	return *(Mtl4Queue*)&queue;
}

inline GpuQueue mtl4HandleToGpuQueue(Mtl4Queue handle) {
	return *(GpuQueue*)&handle;
}

#endif // MTL4_QUEUE_H
