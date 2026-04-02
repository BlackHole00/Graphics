#ifndef GPU_METAL4DEVICE_H
#define GPU_METAL4DEVICE_H

#include <gpu/gpu.h>

void mtl4PrepareAvailableDevicesList(GpuResult* result);

void mtl4EnumerateDevices(GpuDeviceInfo** devices, size_t* devices_count, GpuResult* result);

#endif // GPU_METAL4DEVICE_H
