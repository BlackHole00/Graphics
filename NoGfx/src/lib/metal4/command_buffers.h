#ifndef MTL4_COMMAND_BUFFERS_H
#define MTL4_COMMAND_BUFFERS_H

#include <lib/common/page.h>
#include <lib/common/handle_map.h>
#include <lib/common/storage_sync.h>
#include <lib/metal4/pipelines.h>
#include <lib/metal4/queue.h>

#include <gpu/gpu.h>
#include <Metal/Metal.h>

typedef CmnHandle Mtl4CommandBuffer;

typedef enum Mtl4CommandBufferStatus {
	MTL4_COMMAND_BUFFER_ENCODING,
	MTL4_COMMAND_BUFFER_SUBMITTED,
} Mtl4CommandBufferStatus;

// NOTE: Encoding a command encoder is not thread safe: It can happen from any thread, but sequential encoding is expected. The synchronization is thus expected from the user.
typedef struct Mtl4CommandBufferMetadata {
	Mtl4CommandBufferStatus	status;

	Mtl4Queue			relatedQueue;

	id<MTL4CommandBuffer>		commandBuffer;
	id<MTL4ComputeCommandEncoder>	computeEncoder;
	id<MTL4RenderCommandEncoder>	currentRenderEncoder;

	Mtl4Pipeline	boundPipeline;
	void*		boundTextureHeap;
} Mtl4CommandBufferMetadata;

typedef struct Mtl4CommandBufferStorage {
	CmnPage		page;
	CmnArena	arena;

	id<MTL4CommandAllocator>	commandAllocator;

	CmnHandleMap	<Mtl4CommandBufferMetadata>	commandBuffers;
	CmnStorageSync	sync;
} Mtl4CommandBufferStorage;
extern Mtl4CommandBufferStorage gMtl4CommandBufferStorage;

void mtl4InitCommandBufferStorage(GpuResult* result);
void mtl4FiniCommandBufferStorage(void);

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

void mtl4MemCpy(GpuCommandBuffer cb, void* destGpu, void* srcGpu, size_t size, GpuResult* result);
void mtl4CopyToTexture(GpuCommandBuffer cb, void* destGpu, void* srcGpu, GpuTexture texture, GpuResult* result);
void mtl4CopyFromTexture(GpuCommandBuffer cb, void* destGpu, void* srcGpu, GpuTexture texture, GpuResult* result);

void mtl4SetActiveTextureHeapPtr(GpuCommandBuffer cb, void *ptrGpu, GpuResult* result);

void mtl4Barrier(GpuCommandBuffer cb, GpuStage before, GpuStage after, GpuHazardFlags hazards, GpuResult* result);

Mtl4CommandBuffer mtl4CreateCommandBuffer(GpuResult* result);
// NOTE: Requires deletion-lock on gMtl4CommandBufferStorage.sync.
void mtl4DestroyCommandBuffer(Mtl4CommandBuffer commandBuffer);
bool mtl4IsCommandBufferScheduledForDeletion(Mtl4CommandBuffer commandBuffer);

bool mtl4CanImposeNormalMtlBarrierBetween(GpuStage before, GpuStage after, GpuHazardFlags hazards);
MTLStages mtl4GpuToMtlStage(GpuStage stage, GpuHazardFlags hazards);
MTL4VisibilityOptions mtl4GpuHazardsToMtlVisibilityOptions(GpuHazardFlags hazards);

Mtl4CommandBufferMetadata* mtl4AcquireCommandBufferMetadataFrom(Mtl4CommandBuffer handle);
void mtl4ReleaseCommandBufferMetadata(void);

inline Mtl4CommandBuffer mtl4GpuCommandBufferToHandle(GpuCommandBuffer commandBuffer) {
	return *(Mtl4CommandBuffer*)&commandBuffer;
}
inline GpuCommandBuffer mtl4HandleToGpuCommandBuffer(Mtl4CommandBuffer handle) {
	return *(GpuCommandBuffer*)&handle;
}

#endif // MTL4_COMMAND_BUFFERS_H

