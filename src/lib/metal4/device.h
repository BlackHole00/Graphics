#ifndef GPU_METAL4DEVICE_H
#define GPU_METAL4DEVICE_H

#include <gpu/gpu.h>
#include <Metal/Metal.h>

void mtl4PrepareAvailableDevicesList(GpuResult* result);

void mtl4EnumerateDevices(GpuDeviceInfo** devices, size_t* devices_count, GpuResult* result);
void mtl4SelectDevice(GpuDeviceId deviceId, GpuResult* result);

void mtl4BeginTracing(const char* traceDestinationFile);
void mtl4StopTracing(void);

bool mtl4HasDevice(void);

#endif // GPU_METAL4DEVICE_H

