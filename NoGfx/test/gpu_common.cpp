#include "test.h"

#include <gpu/gpu.h>
#include <cstdio>
#include <cstdlib>
#include <pthread.h>
#include <sched.h>

#include <lib/common/atomic.h>

GpuBackend selectBackendForCurrentPlatform(void) {
	#ifdef __APPLE__
		return GPU_METAL_4;
	#else
		return GPU_NONE;
	#endif
}

GpuBackend selectUnavailableBackendForCurrentPlatform(void) {
	#ifdef __APPLE__
		return GPU_VULKAN;
	#else
		return GPU_METAL_4;
	#endif
}

GpuTextureDesc makeDefaultTextureDesc(void) {
	GpuTextureDesc desc = {};
	desc.type = GPU_TEXTURE_2D;
	desc.dimensions[0] = 64;
	desc.dimensions[1] = 64;
	desc.dimensions[2] = 1;
	desc.mipCount = 1;
	desc.layerCount = 1;
	desc.sampleCount = 1;
	desc.format = GPU_FORMAT_RGBA8_UNORM;
	desc.usage = GPU_USAGE_SAMPLED;

	return desc;
}

bool initGpuAndSelectFirstDevice(GpuResult* result) {
	GpuInitDesc initDesc = {};
	initDesc.backend = selectBackendForCurrentPlatform();
	initDesc.validationEnabled = true;

	gpuInit(&initDesc, result);
	if (*result != GPU_SUCCESS) {
		return false;
	}

	GpuDeviceInfo* devices = nullptr;
	size_t count = 0;
	gpuEnumerateDevices(&devices, &count, result);
	if (*result != GPU_SUCCESS || count == 0 || devices == nullptr) {
		gpuDeinit();
		return false;
	}

	gpuSelectDevice(devices[0].identifier, result);
	if (*result != GPU_SUCCESS) {
		gpuDeinit();
		return false;
	}

	return true;
}

static bool loadBinaryFile(const char* path, uint8_t** data, size_t* size) {
	FILE* file = fopen(path, "rb");
	if (file == nullptr) {
		return false;
	}

	if (fseek(file, 0, SEEK_END) != 0) {
		fclose(file);
		return false;
	}

	long fileSize = ftell(file);
	if (fileSize <= 0) {
		fclose(file);
		return false;
	}

	if (fseek(file, 0, SEEK_SET) != 0) {
		fclose(file);
		return false;
	}

	uint8_t* buffer = (uint8_t*)malloc((size_t)fileSize);
	if (buffer == nullptr) {
		fclose(file);
		return false;
	}

	size_t readSize = fread(buffer, 1, (size_t)fileSize, file);
	fclose(file);
	if (readSize != (size_t)fileSize) {
		free(buffer);
		return false;
	}

	*data = buffer;
	*size = (size_t)fileSize;
	return true;
}

static void freeBinaryFile(uint8_t* data) {
	free(data);
}

typedef struct GpuFunctionConstants {
	float scale;
} GpuFunctionConstants;

typedef struct GpuAllocationCreatorContext {
	size_t bytes;
	size_t align;
	GpuMemory memory;

	void* ptr;
	GpuResult createResult;
	uint32_t created;
} GpuAllocationCreatorContext;

static void* gpuAllocationCreatorThreadProc(void* ptr) {
	GpuAllocationCreatorContext* context = (GpuAllocationCreatorContext*)ptr;

	context->ptr = gpuMalloc(context->bytes, context->align, context->memory, &context->createResult);
	cmnAtomicStore(&context->created, 1u, CMN_RELEASE);

	return nullptr;
}

typedef struct GpuAllocationDestroyerContext {
	GpuAllocationCreatorContext* creator;
	uint32_t destroyed;
} GpuAllocationDestroyerContext;

static void* gpuAllocationDestroyerThreadProc(void* ptr) {
	GpuAllocationDestroyerContext* context = (GpuAllocationDestroyerContext*)ptr;

	while (cmnAtomicLoad(&context->creator->created, CMN_ACQUIRE) == 0u) {
		sched_yield();
	}

	if (context->creator->createResult == GPU_SUCCESS && context->creator->ptr != nullptr) {
		gpuFree(context->creator->ptr);
	}

	cmnAtomicStore(&context->destroyed, 1u, CMN_RELEASE);
	return nullptr;
}

typedef struct GpuTextureCreatorContext {
	GpuTextureDesc desc;

	void* ptrGpu;
	GpuTexture texture;
	GpuResult sizeAlignResult;
	GpuResult allocationResult;
	GpuResult createResult;
	uint32_t created;
} GpuTextureCreatorContext;

static void* gpuTextureCreatorThreadProc(void* ptr) {
	GpuTextureCreatorContext* context = (GpuTextureCreatorContext*)ptr;

	GpuTextureSizeAlign sizeAlign = gpuTextureSizeAlign(&context->desc, &context->sizeAlignResult);
	if (context->sizeAlignResult != GPU_SUCCESS) {
		cmnAtomicStore(&context->created, 1u, CMN_RELEASE);
		return nullptr;
	}

	context->ptrGpu = gpuMalloc(sizeAlign.size, sizeAlign.align, GPU_MEMORY_GPU, &context->allocationResult);
	if (context->allocationResult != GPU_SUCCESS || context->ptrGpu == nullptr) {
		cmnAtomicStore(&context->created, 1u, CMN_RELEASE);
		return nullptr;
	}

	context->texture = gpuCreateTexture(&context->desc, context->ptrGpu, &context->createResult);
	cmnAtomicStore(&context->created, 1u, CMN_RELEASE);

	return nullptr;
}

typedef struct GpuTextureDestroyerContext {
	GpuTextureCreatorContext* creator;
	GpuResult descriptorResult;
	GpuTextureDescriptor descriptor;
	uint32_t destroyed;
} GpuTextureDestroyerContext;

static void* gpuTextureDestroyerThreadProc(void* ptr) {
	GpuTextureDestroyerContext* context = (GpuTextureDestroyerContext*)ptr;

	while (cmnAtomicLoad(&context->creator->created, CMN_ACQUIRE) == 0u) {
		sched_yield();
	}

	if (context->creator->createResult == GPU_SUCCESS && context->creator->texture != 0 && context->creator->ptrGpu != nullptr) {
		GpuViewDesc viewDesc = {};
		viewDesc.format = context->creator->desc.format;
		viewDesc.baseMip = 0;
		viewDesc.mipCount = 1;
		viewDesc.baseLayer = 0;
		viewDesc.layerCount = 1;

		context->descriptor = gpuTextureViewDescriptor(context->creator->texture, &viewDesc, &context->descriptorResult);
		gpuFree(context->creator->ptrGpu);
	}

	cmnAtomicStore(&context->destroyed, 1u, CMN_RELEASE);
	return nullptr;
}

typedef struct GpuStressGate {
	uint32_t ready;
	uint32_t start;
} GpuStressGate;

static void gpuWaitForStressStart(GpuStressGate* gate) {
	cmnAtomicAdd(&gate->ready, 1u, CMN_RELEASE);
	while (cmnAtomicLoad(&gate->start, CMN_ACQUIRE) == 0u) {
		sched_yield();
	}
}

typedef struct GpuAllocationStressThreadContext {
	GpuStressGate* gate;
	size_t iterations;
	GpuMemory memory;
	bool expectHostToDeviceSuccess;
	uint32_t completed;
	uint32_t failed;
} GpuAllocationStressThreadContext;

static void* gpuAllocationStressThreadProc(void* ptr) {
	GpuAllocationStressThreadContext* context = (GpuAllocationStressThreadContext*)ptr;

	gpuWaitForStressStart(context->gate);

	for (size_t i = 0; i < context->iterations; i++) {
		GpuResult allocationResult = GPU_GENERAL_ERROR;
		size_t bytes = 256u + ((i & 7u) * 16u);
		void* allocation = gpuMalloc(bytes, 16, context->memory, &allocationResult);
		if (allocationResult != GPU_SUCCESS || allocation == nullptr) {
			context->failed = 1u;
			context->completed = 1u;
			return nullptr;
		}

		GpuResult pointerResult = GPU_GENERAL_ERROR;
		void* devicePtr = gpuHostToDevicePointer(allocation, &pointerResult);
		if (context->expectHostToDeviceSuccess) {
			if (pointerResult != GPU_SUCCESS || devicePtr == nullptr) {
				gpuFree(allocation);
				context->failed = 1u;
				context->completed = 1u;
				return nullptr;
			}
		} else {
			if (pointerResult != GPU_ALLOCATION_MEMORY_IS_GPU || devicePtr != nullptr) {
				gpuFree(allocation);
				context->failed = 1u;
				context->completed = 1u;
				return nullptr;
			}
		}

		gpuFree(allocation);
	}

	context->completed = 1u;
	return nullptr;
}

typedef struct GpuHostPointerStressThreadContext {
	GpuStressGate* gate;
	void* basePtr;
	uintptr_t expectedOffset;
	size_t iterations;
	uint32_t completed;
	uint32_t failed;
} GpuHostPointerStressThreadContext;

static void* gpuHostPointerStressThreadProc(void* ptr) {
	GpuHostPointerStressThreadContext* context = (GpuHostPointerStressThreadContext*)ptr;

	gpuWaitForStressStart(context->gate);

	for (size_t i = 0; i < context->iterations; i++) {
		GpuResult result = GPU_GENERAL_ERROR;
		void* mappedBase = gpuHostToDevicePointer(context->basePtr, &result);
		if (result != GPU_SUCCESS || mappedBase == nullptr) {
			context->failed = 1u;
			context->completed = 1u;
			return nullptr;
		}

		void* mappedOffset = gpuHostToDevicePointer((void*)((uintptr_t)context->basePtr + context->expectedOffset), &result);
		if (result != GPU_SUCCESS || mappedOffset == nullptr) {
			context->failed = 1u;
			context->completed = 1u;
			return nullptr;
		}

		uintptr_t offset = (uintptr_t)mappedOffset - (uintptr_t)mappedBase;
		if (offset != context->expectedOffset) {
			context->failed = 1u;
			context->completed = 1u;
			return nullptr;
		}
	}

	context->completed = 1u;
	return nullptr;
}

typedef struct GpuTextureStressThreadContext {
	GpuStressGate* gate;
	GpuTextureDesc desc;
	GpuTextureSizeAlign sizeAlign;
	size_t iterations;
	uint32_t completed;
	uint32_t failed;
} GpuTextureStressThreadContext;

static void* gpuTextureStressThreadProc(void* ptr) {
	GpuTextureStressThreadContext* context = (GpuTextureStressThreadContext*)ptr;

	gpuWaitForStressStart(context->gate);

	for (size_t i = 0; i < context->iterations; i++) {
		GpuResult allocationResult = GPU_GENERAL_ERROR;
		void* ptrGpu = gpuMalloc(context->sizeAlign.size, context->sizeAlign.align, GPU_MEMORY_GPU, &allocationResult);
		if (allocationResult != GPU_SUCCESS || ptrGpu == nullptr) {
			context->failed = 1u;
			context->completed = 1u;
			return nullptr;
		}

		GpuResult textureResult = GPU_GENERAL_ERROR;
		GpuTexture texture = gpuCreateTexture(&context->desc, ptrGpu, &textureResult);
		if (textureResult != GPU_SUCCESS || texture == 0) {
			gpuFree(ptrGpu);
			context->failed = 1u;
			context->completed = 1u;
			return nullptr;
		}

		GpuViewDesc viewDesc = {};
		viewDesc.format = context->desc.format;
		viewDesc.baseMip = 0;
		viewDesc.mipCount = 1;
		viewDesc.baseLayer = 0;
		viewDesc.layerCount = 1;

		GpuResult viewResult = GPU_GENERAL_ERROR;
		GpuTextureDescriptor descriptor = gpuTextureViewDescriptor(texture, &viewDesc, &viewResult);
		if (viewResult != GPU_SUCCESS || descriptor.data[0] == 0) {
			gpuFree(ptrGpu);
			context->failed = 1u;
			context->completed = 1u;
			return nullptr;
		}

		GpuResult rwViewResult = GPU_GENERAL_ERROR;
		descriptor = gpuRWTextureViewDescriptor(texture, &viewDesc, &rwViewResult);
		if (rwViewResult != GPU_SUCCESS || descriptor.data[0] == 0) {
			gpuFree(ptrGpu);
			context->failed = 1u;
			context->completed = 1u;
			return nullptr;
		}

		gpuFree(ptrGpu);
	}

	context->completed = 1u;
	return nullptr;
}

typedef struct GpuPipelineStressThreadContext {
	GpuStressGate* gate;
	const uint8_t* computeIr;
	size_t computeIrSize;
	const uint8_t* vertexIr;
	size_t vertexIrSize;
	const uint8_t* fragmentIr;
	size_t fragmentIrSize;
	const uint8_t* meshletIr;
	size_t meshletIrSize;
	GpuFunctionConstants constants;
	size_t iterations;
	uint32_t completed;
	uint32_t failed;
} GpuPipelineStressThreadContext;

static void* gpuPipelineStressThreadProc(void* ptr) {
	GpuPipelineStressThreadContext* context = (GpuPipelineStressThreadContext*)ptr;

	gpuWaitForStressStart(context->gate);

	for (size_t i = 0; i < context->iterations; i++) {
		GpuResult pipelineResult = GPU_GENERAL_ERROR;
		GpuPipeline pipeline = 0;

		switch (i % 3u) {
			case 0u:
				pipeline = gpuCreateComputePipeline(context->computeIr, context->computeIrSize, &context->constants, sizeof(context->constants), &pipelineResult);
				break;
			case 1u:
				pipeline = gpuCreateRenderPipeline(
					context->vertexIr, context->vertexIrSize,
					context->fragmentIr, context->fragmentIrSize,
					&context->constants, sizeof(context->constants),
					&context->constants, sizeof(context->constants),
					&pipelineResult
				);
				break;
			default:
				pipeline = gpuCreateMeshletPipeline(
					context->meshletIr, context->meshletIrSize,
					context->fragmentIr, context->fragmentIrSize,
					&context->constants, sizeof(context->constants),
					&context->constants, sizeof(context->constants),
					&pipelineResult
				);
				break;
		}

		if (pipelineResult != GPU_SUCCESS || pipeline == 0) {
			context->failed = 1u;
			context->completed = 1u;
			return nullptr;
		}

		gpuFreePipeline(pipeline);
	}

	context->completed = 1u;
	return nullptr;
}

static void waitForGateReady(GpuStressGate* gate, size_t threadCount) {
	for (;;) {
		if (cmnAtomicLoad(&gate->ready, CMN_ACQUIRE) == threadCount) {
			break;
		}
		sched_yield();
	}
}

static void runGpuAllocationStress(Test* test, GpuMemory memory, bool expectHostToDeviceSuccess) {
	GpuResult result;
	if (!initGpuAndSelectFirstDevice(&result)) {
		TEST_ASSERT(test, result == GPU_SUCCESS);
		return;
	}

	const size_t threadCount = 8;
	const size_t iterations = 256;

	GpuStressGate gate = {};
	GpuAllocationStressThreadContext contexts[threadCount] = {};
	pthread_t threads[threadCount];

	for (size_t i = 0; i < threadCount; i++) {
		contexts[i].gate = &gate;
		contexts[i].iterations = iterations;
		contexts[i].memory = memory;
		contexts[i].expectHostToDeviceSuccess = expectHostToDeviceSuccess;

		int createResult = pthread_create(&threads[i], nullptr, gpuAllocationStressThreadProc, &contexts[i]);
		TEST_ASSERT(test, createResult == 0);
	}

	waitForGateReady(&gate, threadCount);
	cmnAtomicStore(&gate.start, 1u, CMN_RELEASE);

	for (size_t i = 0; i < threadCount; i++) {
		int joinResult = pthread_join(threads[i], nullptr);
		TEST_ASSERT(test, joinResult == 0);
		TEST_ASSERT(test, contexts[i].completed == 1u);
		TEST_ASSERT(test, contexts[i].failed == 0u);
	}

	gpuDeinit();
}

static void runGpuHostPointerStress(Test* test) {
	GpuResult result;
	if (!initGpuAndSelectFirstDevice(&result)) {
		TEST_ASSERT(test, result == GPU_SUCCESS);
		return;
	}

	void* ptrCpu = gpuMalloc(512, 16, GPU_MEMORY_DEFAULT, &result);
	TEST_ASSERT(test, result == GPU_SUCCESS);
	TEST_ASSERT(test, ptrCpu != nullptr);

	const size_t threadCount = 6;
	const size_t iterations = 1024;

	GpuStressGate gate = {};
	GpuHostPointerStressThreadContext contexts[threadCount] = {};
	pthread_t threads[threadCount];

	for (size_t i = 0; i < threadCount; i++) {
		contexts[i].gate = &gate;
		contexts[i].basePtr = ptrCpu;
		contexts[i].expectedOffset = 128u;
		contexts[i].iterations = iterations;

		int createResult = pthread_create(&threads[i], nullptr, gpuHostPointerStressThreadProc, &contexts[i]);
		TEST_ASSERT(test, createResult == 0);
	}

	waitForGateReady(&gate, threadCount);
	cmnAtomicStore(&gate.start, 1u, CMN_RELEASE);

	for (size_t i = 0; i < threadCount; i++) {
		int joinResult = pthread_join(threads[i], nullptr);
		TEST_ASSERT(test, joinResult == 0);
		TEST_ASSERT(test, contexts[i].completed == 1u);
		TEST_ASSERT(test, contexts[i].failed == 0u);
	}

	gpuFree(ptrCpu);

	gpuDeinit();
}

static void runGpuTextureStress(Test* test) {
	GpuResult result;
	if (!initGpuAndSelectFirstDevice(&result)) {
		TEST_ASSERT(test, result == GPU_SUCCESS);
		return;
	}

	GpuTextureDesc desc = makeDefaultTextureDesc();
	GpuTextureSizeAlign sizeAlign = gpuTextureSizeAlign(&desc, &result);
	TEST_ASSERT(test, result == GPU_SUCCESS);
	TEST_ASSERT(test, sizeAlign.size > 0);
	TEST_ASSERT(test, sizeAlign.align > 0);

	const size_t threadCount = 6;
	const size_t iterations = 64;

	GpuStressGate gate = {};
	GpuTextureStressThreadContext contexts[threadCount] = {};
	pthread_t threads[threadCount];

	for (size_t i = 0; i < threadCount; i++) {
		contexts[i].gate = &gate;
		contexts[i].desc = desc;
		contexts[i].sizeAlign = sizeAlign;
		contexts[i].iterations = iterations;

		int createResult = pthread_create(&threads[i], nullptr, gpuTextureStressThreadProc, &contexts[i]);
		TEST_ASSERT(test, createResult == 0);
	}

	waitForGateReady(&gate, threadCount);
	cmnAtomicStore(&gate.start, 1u, CMN_RELEASE);

	for (size_t i = 0; i < threadCount; i++) {
		int joinResult = pthread_join(threads[i], nullptr);
		TEST_ASSERT(test, joinResult == 0);
		TEST_ASSERT(test, contexts[i].completed == 1u);
		TEST_ASSERT(test, contexts[i].failed == 0u);
	}

	gpuDeinit();
}

static void runGpuPipelineStress(Test* test) {
	GpuResult result;
	if (!initGpuAndSelectFirstDevice(&result)) {
		TEST_ASSERT(test, result == GPU_SUCCESS);
		return;
	}

	uint8_t* computeIr = nullptr;
	size_t computeIrSize = 0;
	TEST_ASSERT(test, loadBinaryFile("build/compute_constants.metallib", &computeIr, &computeIrSize));

	uint8_t* vertexIr = nullptr;
	size_t vertexIrSize = 0;
	TEST_ASSERT(test, loadBinaryFile("build/render_vertex_constants.metallib", &vertexIr, &vertexIrSize));

	uint8_t* fragmentIr = nullptr;
	size_t fragmentIrSize = 0;
	TEST_ASSERT(test, loadBinaryFile("build/render_fragment_constants.metallib", &fragmentIr, &fragmentIrSize));

	uint8_t* meshletIr = nullptr;
	size_t meshletIrSize = 0;
	TEST_ASSERT(test, loadBinaryFile("build/meshlet_constants.metallib", &meshletIr, &meshletIrSize));

	const size_t threadCount = 6;
	const size_t iterations = 96;

	GpuStressGate gate = {};
	GpuPipelineStressThreadContext contexts[threadCount] = {};
	pthread_t threads[threadCount];

	for (size_t i = 0; i < threadCount; i++) {
		contexts[i].gate = &gate;
		contexts[i].computeIr = computeIr;
		contexts[i].computeIrSize = computeIrSize;
		contexts[i].vertexIr = vertexIr;
		contexts[i].vertexIrSize = vertexIrSize;
		contexts[i].fragmentIr = fragmentIr;
		contexts[i].fragmentIrSize = fragmentIrSize;
		contexts[i].meshletIr = meshletIr;
		contexts[i].meshletIrSize = meshletIrSize;
		contexts[i].constants.scale = 1.5f;
		contexts[i].iterations = iterations;

		int createResult = pthread_create(&threads[i], nullptr, gpuPipelineStressThreadProc, &contexts[i]);
		TEST_ASSERT(test, createResult == 0);
	}

	waitForGateReady(&gate, threadCount);
	cmnAtomicStore(&gate.start, 1u, CMN_RELEASE);

	for (size_t i = 0; i < threadCount; i++) {
		int joinResult = pthread_join(threads[i], nullptr);
		TEST_ASSERT(test, joinResult == 0);
		TEST_ASSERT(test, contexts[i].completed == 1u);
		TEST_ASSERT(test, contexts[i].failed == 0u);
	}

	freeBinaryFile(computeIr);
	freeBinaryFile(vertexIr);
	freeBinaryFile(fragmentIr);
	freeBinaryFile(meshletIr);

	gpuDeinit();
}

