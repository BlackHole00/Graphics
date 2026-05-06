#include "queue.h"

#include <lib/metal4/context.h>

GpuQueue mtl4CreateQueue(GpuResult* result) {
	id<MTL4CommandQueue> queue = cmnAtomicLoad(&gMtl4Context.queue);
	if (queue != nil) {
		CMN_SET_RESULT(result, GPU_SUCCESS);
		return MTL4_QUEUE_HANDLE;
	}

	queue = [gMtl4Context.device newMTL4CommandQueue];
	if (queue == nil) {
		CMN_SET_RESULT(result, GPU_OUT_OF_GPU_MEMORY);
		return 0;
	}

	if (!cmnAtomicCompareExchangeStrong(&gMtl4Context.queue, (id<MTL4CommandQueue>)nil, queue)) {
		[queue release];
	}

	CMN_SET_RESULT(result, GPU_SUCCESS);
	return MTL4_QUEUE_HANDLE;
}

id<MTL4CommandQueue> mtl4Queue(void) {
	return cmnAtomicLoad(&gMtl4Context.queue);
}

bool mtl4IsQueueValid(GpuQueue queue) {
	return queue == MTL4_QUEUE_HANDLE;
}
