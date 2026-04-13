#include <cassert>
#include <gpu/gpu.h>

#include <lib/common/atomic.h>
#include <lib/common/futex.h>
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

	int a;
	cmnAtomicStore(&a, 10);
	cmnAtomicCompareExchangeStrong(&a, 10, 20);
	assert(cmnAtomicExchange(&a, 30) == 20);
	assert(cmnAtomicLoad(&a) == 30);

	CmnFutex futex = 0;
	cmnFutexWaitWithTimeout(&futex, 0, 1000000000);

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


