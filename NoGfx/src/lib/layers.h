#ifndef GPU_LAYERS_H
#define GPU_LAYERS_H

#include <gpu/gpu.h>
#include <lib/common/common.h>

#define GPU_MAX_LAYERS 4

typedef struct GpuBaseLayer {
	void (*layerInit)(GpuResult* result);
	void (*gpuDeinit)(void);

	void (*gpuEnumerateDevices)(GpuDeviceInfo** devices, size_t* devices_count, GpuResult* result);
	void (*gpuSelectDevice)(GpuDeviceId deviceId, GpuResult* result);

	void* (*gpuMalloc)(size_t bytes, size_t align, GpuMemory memory, GpuResult* result);
	void  (*gpuFree)(void* ptr);
	void* (*gpuHostToDevicePointer)(void* ptr, GpuResult* result);
} GpuBaseLayer;

typedef struct {
	const GpuLayer*		validationLayers[GPU_MAX_LAYERS];
	size_t			validationLayerCount;

	const GpuBaseLayer*	baseLayer;
} GpuActiveLayers;
extern GpuActiveLayers gGpuActiveLayers;

#define GPU_LAYERED_CALL_NO_PARAMS(_function)					\
for (size_t _i = gGpuActiveLayers.validationLayerCount; _i > 0; _i--) {		\
	auto function = gGpuActiveLayers.validationLayers[_i - 1]->_function;	\
	if (function != nullptr) {						\
		bool ok = function();						\
		if (!ok) {							\
			break;							\
		}								\
	}									\
}										\
return gGpuActiveLayers.baseLayer->_function()

#define GPU_LAYERED_CALL(_function, ...)					\
for (size_t _i = gGpuActiveLayers.validationLayerCount; _i > 0; _i--) {		\
	auto function = gGpuActiveLayers.validationLayers[_i - 1]->_function;	\
	if (function != nullptr) {						\
		bool ok = function(__VA_ARGS__);				\
		if (!ok) {							\
			break;							\
		}								\
	}									\
}										\
return gGpuActiveLayers.baseLayer->_function(__VA_ARGS__)

bool gpuPushLayer(const GpuLayer* layer);

// NOTE: Platform specific
GpuBaseLayer* gpuAcquireBaseLayerFor(GpuBackend backend);

// NOTE: Platform specific
GpuLayer* gpuAcquireValidationLayerFor(GpuBackend backend);

#endif // GPU_LAYERS_H

