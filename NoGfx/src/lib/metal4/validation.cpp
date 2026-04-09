#include "validation.h"

#include <lib/metal4/context.h>

bool mtl4ValidateEnumerateDevices(GpuDeviceInfo** devices, size_t* devices_count, GpuResult* result) {
	if (devices == nullptr || devices_count == nullptr) {
		CMN_SET_RESULT(result, GPU_INVALID_PARAMETERS);
		return false;
	}

	CMN_SET_RESULT(result, GPU_SUCCESS);
	return true;
}

bool mtl4ValidateSelectDevice(GpuDeviceId deviceId, GpuResult* result) {
	if (deviceId >= gMtl4Context.availableDevices.count) {
		CMN_SET_RESULT(result, GPU_INVALID_DEVICE);
		return false;
	}

	if (gMtl4Context.device != nullptr) {
		CMN_SET_RESULT(result, GPU_DEVICE_ALREADY_SELECTED);
		return false;
	}

	CMN_SET_RESULT(result, GPU_SUCCESS);
	return true;
}

