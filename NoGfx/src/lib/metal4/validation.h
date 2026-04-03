#ifndef GPU_METAL4VALIDATION_H
#define GPU_METAL4VALIDATION_H

#include <gpu/gpu.h>

void mtl4ValidateEnumerateDevices(GpuDeviceInfo** devices, size_t* devices_count, GpuResult* result);
void mtl4ValidateSelectDevice(GpuDeviceId deviceId, GpuResult* result);

#endif // GPU_METAL4_VALIDATION_H

