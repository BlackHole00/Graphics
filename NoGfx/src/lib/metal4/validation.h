#ifndef GPU_METAL4VALIDATION_H
#define GPU_METAL4VALIDATION_H

#include <gpu/gpu.h>

bool mtl4ValidateEnumerateDevices(GpuDeviceInfo** devices, size_t* devices_count, GpuResult* result);
bool mtl4ValidateSelectDevice(GpuDeviceId deviceId, GpuResult* result);

bool mtl4ValidateGpuMalloc(size_t bytes, size_t align, GpuMemory memory, GpuResult* result);
bool mtl4ValidateGpuHostToDevicePointer(void* ptr, GpuResult* result);

bool mtl4ValidateTextureDesc(const GpuTextureDesc* desc, GpuResult* result);

bool mtl4ValidateGpuTextureSizeAndAlign(const GpuTextureDesc* desc, GpuResult* result);
bool mtl4ValidateGpuCreateTexture(const GpuTextureDesc* desc, void* ptrGpu, GpuResult* result);
bool mtl4ValidateGpuTextureViewDescriptor(GpuTexture texture, const GpuViewDesc* desc, GpuResult* result);
bool mtl4ValidateGpuTextureRWViewDescriptor(GpuTexture texture, const GpuViewDesc* desc, GpuResult* result);

#endif // GPU_METAL4_VALIDATION_H

