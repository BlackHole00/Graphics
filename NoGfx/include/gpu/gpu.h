#ifndef GFX_GFX_H
#define GFX_GFX_H

#ifdef __cplusplus
extern "C" {
#endif // __cplusplus

#include <stddef.h>
#include <stdbool.h>

typedef enum GpuResult {
	GPU_SUCCESS = 0,

	GPU_BACKEND_NOT_SUPPORTED,
	GPU_TOO_MANY_LAYERS,
	GPU_INVALID_DEVICE,
	GPU_DEVICE_ALREADY_SELECTED,
	GPU_OUT_OF_CPU_MEMORY,
	GPU_OUT_OF_GPU_MEMORY,

	GPU_INVALID_PARAMETERS,

	GPU_GENERAL_ERROR,
} GpuResult;

typedef enum GpuBackend {
	GPU_NONE = 0,
	GPU_METAL_4,
	GPU_VULKAN,
	// ... 
} GpuBackend;

typedef enum GpuDeviceType {
	GPU_INTEGRATED = 0,
	GPU_DEDICATED,
} GpuDeviceType;

typedef enum GpuMemory {
	GPU_MEMORY_DEFAULT = 0,
	GPU_MEMORY_GPU,
	GPU_MEMORY_READBACK,
} GpuMemory;

typedef size_t GpuDeviceId;

typedef struct GpuDeviceInfo {
	GpuDeviceId identifier;
	const char* name;
	const char* vendor;
	GpuDeviceType type;
	// TODO: device capabilities, limits, etc...
} GpuDeviceInfo;


typedef struct GpuLayer {
	bool (*layerInit)(GpuResult* result);
	bool (*gpuDeinit)(void);

	bool (*gpuEnumerateDevices)(GpuDeviceInfo** devices, size_t* devices_count, GpuResult* result);
	bool (*gpuSelectDevice)(GpuDeviceId deviceId, GpuResult* result);

	bool (*gpuMalloc)(size_t bytes, size_t align, GpuMemory memory, GpuResult* result);
	bool (*gpuFree)(void* ptr);
	bool (*gpuHostToDevicePointer)(void* ptr, GpuResult* result);
} GpuLayer;

typedef struct GpuInitDesc {
	GpuBackend backend;
	bool validationEnabled;
	GpuLayer* extraLayers;
	size_t extraLayerCount;
} GpuInitDesc;

void gpuInit(const GpuInitDesc* desc, GpuResult* result);
void gpuDeinit(void);

void gpuEnumerateDevices(GpuDeviceInfo** devices, size_t* devices_count, GpuResult* result);
void gpuSelectDevice(GpuDeviceId deviceId, GpuResult* result);

void* gpuMalloc(size_t bytes, size_t align, GpuMemory memory, GpuResult* result);
void  gpuFree(void* ptr);
void* gpuHostToDevicePointer(void* ptr, GpuResult* result);

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif // GFX_GFX_H
