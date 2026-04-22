#include <cassert>
#include <gpu/gpu.h>

#include <lib/layers.h>

void gpuInit(const GpuInitDesc* desc, GpuResult* result) {
	if (desc->extraLayerCount > GPU_MAX_LAYERS - 2) {
		CMN_SET_RESULT(result, GPU_TOO_MANY_LAYERS);
		return;
	}

	const GpuBaseLayer* baseLayer = gpuAcquireBaseLayerFor(desc->backend);
	if (baseLayer == nullptr) {
		CMN_SET_RESULT(result, GPU_BACKEND_NOT_SUPPORTED);
		return;
	}
	gGpuActiveLayers.baseLayer = baseLayer;

	if (desc->validationEnabled) {
		const GpuLayer* validationLayer	= gpuAcquireValidationLayerFor(desc->backend);
		if (validationLayer == nullptr) {
			CMN_SET_RESULT(result, GPU_BACKEND_NOT_SUPPORTED);
			return;
		}
		gpuPushLayer(validationLayer);
	}

	GPU_LAYERED_CALL(layerInit, result);
}

void gpuDeinit(void) {
	GPU_LAYERED_CALL_NO_PARAMS_NO_RETURN(gpuDeinit);

	gGpuActiveLayers = {};
}

void gpuEnumerateDevices(GpuDeviceInfo **devices, size_t *devices_count, GpuResult *result) {
	GPU_LAYERED_CALL(gpuEnumerateDevices, devices, devices_count, result);
}

void gpuSelectDevice(GpuDeviceId deviceId, GpuResult* result) {
	GPU_LAYERED_CALL(gpuSelectDevice, deviceId, result);
}

void* gpuMalloc(size_t bytes, size_t align, GpuMemory memory, GpuResult* result) {
	GPU_LAYERED_CALL(gpuMalloc, bytes, align, memory, result);

	return nullptr;
}

void  gpuFree(void* ptr) {
	GPU_LAYERED_CALL(gpuFree, ptr);
}

void* gpuHostToDevicePointer(void* ptr, GpuResult* result) {
	GPU_LAYERED_CALL(gpuHostToDevicePointer, ptr, result);

	return nullptr;
}

GpuTextureSizeAlign gpuTextureSizeAlign(const GpuTextureDesc* desc, GpuResult* result) {
	GPU_LAYERED_CALL(gpuTextureSizeAlign, desc, result);

	return {};
}

GpuTexture gpuCreateTexture(const GpuTextureDesc* desc, void* ptrGpu, GpuResult* result) {
	GPU_LAYERED_CALL(gpuCreateTexture, desc, ptrGpu, result);

	return 0;
}

GpuTextureDescriptor gpuTextureViewDescriptor(GpuTexture texture, const GpuViewDesc* desc, GpuResult* result) {
	GPU_LAYERED_CALL(gpuTextureViewDescriptor, texture, desc, result);

	return {};
}

GpuTextureDescriptor gpuRWTextureViewDescriptor(GpuTexture texture, const GpuViewDesc* desc, GpuResult* result) {
	GPU_LAYERED_CALL(gpuRWTextureViewDescriptor, texture, desc, result);

	return {};
}

GpuPipeline gpuCreateComputePipeline(
	const uint8_t* ir, size_t irSize,
	const void* constants, size_t constantsSize,
	GpuResult* result
) {
	GPU_LAYERED_CALL(gpuCreateComputePipeline, ir, irSize, constants, constantsSize, result);

	return {};
}

GpuPipeline gpuCreateRenderPipeline(
	const uint8_t* vertexIr, size_t vertexIrSize,
	const uint8_t* fragmentIr, size_t fragmentIrSize,
	const void* vertexConstants, size_t vertexConstantsSize,
	const void* fragmentConstants, size_t fragmentConstantsSize,
	GpuResult* result
) {
	GPU_LAYERED_CALL(
		gpuCreateRenderPipeline,
		vertexIr, vertexIrSize,
		fragmentIr, fragmentIrSize,
		vertexConstants, vertexConstantsSize,
		fragmentConstants, fragmentConstantsSize,
		result
	);

	return {};
}

GpuPipeline gpuCreateMeshletPipeline(
	const uint8_t* meshletIr, size_t meshletIrSize,
	const uint8_t* fragmentIr, size_t fragmentIrSize,
	const void* meshletConstants, size_t meshletConstantsSize,
	const void* fragmentConstants, size_t fragmentConstantsSize,
	GpuResult* result
) {
	GPU_LAYERED_CALL(
		gpuCreateMeshletPipeline,
		meshletIr, meshletIrSize,
		fragmentIr, fragmentIrSize,
		meshletConstants, meshletConstantsSize,
		fragmentConstants, fragmentConstantsSize,
		result
	)

	return {};
}

void gpuFreePipeline(GpuPipeline pipeline) {
	GPU_LAYERED_CALL(gpuFreePipeline, pipeline);
}

GpuQueue gpuCreateQueue(GpuResult* result) {
	GPU_LAYERED_CALL(gpuCreateQueue, result);

	return {};
}

GpuCommandBuffer gpuStartCommandEncoding(GpuQueue queue, GpuResult* result) {
	GPU_LAYERED_CALL(gpuStartCommandEncoding, queue, result);

	return {};
}

void gpuSubmit(GpuQueue queue, GpuCommandBuffer* commandBuffers, size_t commandBufferCount, GpuResult* result) {
	GPU_LAYERED_CALL(gpuSubmit, queue, commandBuffers, commandBufferCount, result);
}

void gpuSubmitWithSignal(
	GpuQueue queue,
	GpuCommandBuffer* commandBuffers,
	size_t commandBufferCount,
	GpuSemaphore semaphore,
	uint64_t value,
	GpuResult* result
) {
	GPU_LAYERED_CALL(gpuSubmitWithSignal, queue, commandBuffers, commandBufferCount, semaphore, value, result);
}
