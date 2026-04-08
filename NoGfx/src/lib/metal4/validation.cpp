#include "validation.h"

#include <lib/metal4/context.h>

void mtl4ValidateEnumerateDevices(GpuDeviceInfo** devices, size_t* devices_count, GpuResult* result) {
	if (devices == nullptr || devices_count == nullptr) {
		*result = GPU_INVALID_PARAMETERS;
	}

	*result = GPU_SUCCESS;
}

void mtl4ValidateSelectDevice(GpuDeviceId deviceId, GpuResult* result) {
	if (deviceId >= gMtl4Context.availableDevices.count) {
		*result = GPU_INVALID_DEVICE;
		return;
	}

	if (gMtl4Context.device != nullptr) {
		*result = GPU_DEVICE_ALREADY_SELECTED;
		return;
	}

	*result = GPU_SUCCESS;
}

