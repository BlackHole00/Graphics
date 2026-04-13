#ifndef GPU_METAL4VALIDATION_H
#define GPU_METAL4VALIDATION_H

#include <gpu/gpu.h>

bool mtl4ValidateEnumerateDevices(GpuDeviceInfo** devices, size_t* devices_count, GpuResult* result);
bool mtl4ValidateSelectDevice(GpuDeviceId deviceId, GpuResult* result);

bool mtl4ValidateGpuMalloc(size_t bytes, size_t align, GpuMemory memory, GpuResult* result);
bool mtl4ValidateGpuHostToDevicePointer(void* ptr, GpuResult* result);

#endif // GPU_METAL4_VALIDATION_H

