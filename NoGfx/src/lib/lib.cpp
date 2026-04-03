#include <gpu/gpu.h>

#include <lib/layers.h>

// TODO: Move to platform dependent code
void gpuInit(const GpuInitDesc* desc, GpuResult* result) {
	if (desc->extraLayerCount > GPU_MAX_LAYERS - 2) {
		*result = GPU_TOO_MANY_LAYERS;
		return;
	}

	const GpuLayer* baseLayer	= gpuAcquireBaseLayerFor(desc->backend);
	if (baseLayer == nullptr) {
		*result = GPU_BACKEND_NOT_SUPPORTED;
		return;
	}

	const GpuLayer* validationLayer	= gpuAcquireValidationLayerFor(desc->backend);
	if (validationLayer == nullptr) {
		*result = GPU_BACKEND_NOT_SUPPORTED;
		return;
	}

	gpuPushLayer(baseLayer);
	gpuPushLayer(validationLayer);

	GPU_LAYERED_CALL_NO_PARAMS(layerInit, result);
	if (*result != GPU_SUCCESS) {
		return;
	}

	*result = GPU_SUCCESS;
}

void gpuDeinit(void) {
	GPU_LAYERED_CALL_NO_RESULT(gpuDeinit);
}

void gpuEnumerateDevices(GpuDeviceInfo **devices, size_t *devices_count, GpuResult *result) {
	GPU_LAYERED_CALL(gpuEnumerateDevices, result, devices, devices_count);
}

void gpuSelectDevice(GpuDeviceId deviceId, GpuResult* result) {
	GPU_LAYERED_CALL(gpuSelectDevice, result, deviceId);
}

