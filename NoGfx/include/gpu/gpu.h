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
	GPU_OUT_OF_CPU_MEMORY,
	GPU_OUT_OF_GPU_MEMORY,

	GPU_GENERAL_ERROR,
} GpuResult;

typedef enum GpuBackend {
	GPU_NONE = 0,
	GPU_METAL_4,
	GPU_VULKAN,
	// ... 
} GpuBackend;

typedef enum GpuDeviceType {
	GPU_INTEGRATED,
	GPU_DEDICATED,
} GpuDeviceType;

typedef size_t GpuDeviceId;

typedef struct GpuDeviceInfo {
	GpuDeviceId identifier;
	const char* name;
	const char* vendor;
	GpuDeviceType type;
	// TODO: device capabilities, limits, etc...
} GpuDeviceInfo;

typedef struct GpuLayer {
	void (*layerInit)(GpuResult* result);
	void (*gpuDeinit)(void);

	void (*gpuEnumerateDevices)(GpuDeviceInfo** devices, size_t* devices_count, GpuResult* result);
	void (*gpuSelectDevice)(GpuDeviceId deviceId, GpuResult* result);
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

#ifdef __cplusplus
} // extern "C"
#endif // __cplusplus

#endif // GFX_GFX_H
