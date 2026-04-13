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

