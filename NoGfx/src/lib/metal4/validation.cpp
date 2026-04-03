#include "validation.h"

#include <lib/metal4/device.h>

void mtl4ValidateEnumerateDevices(GpuDeviceInfo** devices, size_t* devices_count, GpuResult* result) {
	if (devices == nullptr || devices_count == nullptr) {
		*result = GPU_INVALID_PARAMETERS;
	}
}

void mtl4ValidateSelectDevice(GpuDeviceId deviceId, GpuResult* result) {
	if (deviceId >= gMtl4AvailableDevicesList.count) {
		*result = GPU_INVALID_DEVICE;
	}
}

