#ifndef MTL4_COMMAND_BUFFERS_H
#define MTL4_COMMAND_BUFFERS_H

#include <lib/common/page.h>
#include <lib/common/handle_map.h>
#include <lib/common/storage_sync.h>
#include <lib/common/keyed_chain.h>
#include <lib/metal4/pipelines.h>
#include <lib/metal4/queue.h>

#include <gpu/gpu.h>
#include <Metal/Metal.h>

// NOTE: Make a single pool element fit in 2 cache lines
#define MTL4_COMMANDBUFFERSTORAGE_POOLSIZE 256

typedef CmnHandle Mtl4CommandBuffer;

typedef enum Mtl4CommandBufferStatus {
	MTL4_COMMAND_BUFFER_ENCODING,
	MTL4_COMMAND_BUFFER_SUBMITTED,
} Mtl4CommandBufferStatus;

typedef struct Mtl4FenceId {
	void* ptrGpu;
	uint64_t value;
} Mtl4FenceId;

// NOTE: Encoding a command encoder is not thread safe: It can happen from any thread, but sequential encoding
//	is expected. The synchronization is thus expected from the user.
typedef struct Mtl4CommandBufferMetadata {
	Mtl4CommandBufferStatus	status;

	Mtl4Queue			relatedQueue;

	id<MTL4CommandBuffer>		commandBuffer;
	id<MTL4ComputeCommandEncoder>	computeEncoder;
	id<MTL4RenderCommandEncoder>	renderEncoder;

	Mtl4Pipeline	boundPipeline;
	void*		boundTextureHeap;

	CmnKeyedChain<Mtl4FenceId, id<MTLFence>, 10>	computeFences;
	CmnKeyedChain<Mtl4FenceId, id<MTLFence>, 10>	renderFences;
} Mtl4CommandBufferMetadata;
static_assert(sizeof(CmnKeyedChainNode<Mtl4FenceId, id<MTLFence>, 10>) <= MTL4_COMMANDBUFFERSTORAGE_POOLSIZE, "A fence list node should fit into the pool allocator.");

typedef struct Mtl4CommandBufferStorage {
	CmnPage		arenaPage;
	CmnPage		poolPage;

	CmnArena	arena;
	CmnAllocator	arenaAllocator;
	CmnPool		pool;
	CmnAllocator	poolAllocator;

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
void mtl4SignalAfter(GpuCommandBuffer cb, GpuStage before, void* ptrGpu, uint64_t value, GpuSignal signal, GpuResult* result);
void mtl4WaitBefore(GpuCommandBuffer cb, GpuStage after, void* ptrGpu, uint64_t value, GpuOp op, GpuHazardFlags hazards, uint64_t mask, GpuResult* result);

Mtl4CommandBuffer mtl4CreateCommandBuffer(GpuResult* result);
// NOTE: Requires deletion-lock on gMtl4CommandBufferStorage.sync.
void mtl4DestroyCommandBuffer(Mtl4CommandBuffer commandBuffer);
bool mtl4IsCommandBufferScheduledForDeletion(Mtl4CommandBuffer commandBuffer);

bool mtl4IsStageCompute(GpuStage stage);
bool mtl4IsStageRender(GpuStage stage);

bool mtl4CanImposeNormalMtlBarrierBetween(GpuStage before, GpuStage after, GpuHazardFlags hazards);
MTLStages mtl4GpuToMtlStage(GpuStage stage);
MTL4VisibilityOptions mtl4GpuHazardsToMtlVisibilityOptions(GpuHazardFlags hazards);

id<MTLFence> mtl4GetOrCreateComputeFence(Mtl4CommandBufferMetadata* metadata, void* ptrGpu, uint64_t value, GpuResult* result);
id<MTLFence> mtl4GetOrCreateRenderFence(Mtl4CommandBufferMetadata* metadata, void* ptrGpu, uint64_t value, GpuResult* result);

Mtl4CommandBufferMetadata* mtl4AcquireCommandBufferMetadataFrom(Mtl4CommandBuffer handle);
void mtl4ReleaseCommandBufferMetadata(void);

inline Mtl4CommandBuffer mtl4GpuCommandBufferToHandle(GpuCommandBuffer commandBuffer) {
	return *(Mtl4CommandBuffer*)&commandBuffer;
}
inline GpuCommandBuffer mtl4HandleToGpuCommandBuffer(Mtl4CommandBuffer handle) {
	return *(GpuCommandBuffer*)&handle;
}

template <>
struct CmnTypeTraits<Mtl4FenceId> {
	static bool eq(const Mtl4FenceId& left, const Mtl4FenceId& right) {
		return left.ptrGpu == right.ptrGpu && left.value == right.value;
	}

	static CmnCmp cmp(const Mtl4FenceId& left, const Mtl4FenceId& right) {
		(void)left; (void)right;
		assert(false && "Unimplemented.");
	}

	static size_t hash(const Mtl4FenceId& value) {
		(void)value;
		assert(false && "Unimplemented.");
	}
};

#endif // MTL4_COMMAND_BUFFERS_H

