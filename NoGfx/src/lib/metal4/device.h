#ifndef GPU_METAL4DEVICE_H
#define GPU_METAL4DEVICE_H

#include <gpu/gpu.h>
#include <Metal/Metal.h>

typedef struct {
	GpuDeviceInfo*	info;
	id<MTLDevice>*	devices;

	size_t		count;
} Mtl4AvailableDevicesList;
extern Mtl4AvailableDevicesList gMtl4AvailableDevicesList;

void mtl4PrepareAvailableDevicesList(GpuResult* result);

void mtl4EnumerateDevices(GpuDeviceInfo** devices, size_t* devices_count, GpuResult* result);
void mtl4SelectDevice(GpuDeviceId deviceId, GpuResult* result);

#endif // GPU_METAL4DEVICE_H

