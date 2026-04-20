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

	GpuTextureSizeAlign (*gpuTextureSizeAlign)(const GpuTextureDesc* desc, GpuResult* result);
	GpuTexture (*gpuCreateTexture)(const GpuTextureDesc* desc, void* ptrGpu, GpuResult* result);
	GpuTextureDescriptor (*gpuTextureViewDescriptor)(GpuTexture texture, const GpuViewDesc* desc, GpuResult* result);
	GpuTextureDescriptor (*gpuRWTextureViewDescriptor)(GpuTexture texture, const GpuViewDesc* desc, GpuResult* result);

	GpuPipeline (*gpuCreateComputePipeline)(uint8_t* bytes, size_t size, GpuResult* result);
	GpuPipeline (*gpuCreateRenderPipeline)(uint8_t* bytes, size_t size, GpuResult* result);
	GpuPipeline (*gpuCreateMeshletPipeline)(uint8_t* bytes, size_t size, GpuResult* result);
	void (*gpuFreePipeline)(GpuPipeline pipeline);
} GpuBaseLayer;

typedef struct {
	const GpuLayer*		validationLayers[GPU_MAX_LAYERS];
	size_t			validationLayerCount;

	const GpuBaseLayer*	baseLayer;
} GpuActiveLayers;
extern GpuActiveLayers gGpuActiveLayers;

#define GPU_LAYERED_CALL_NO_PARAMS(_function)						\
do {											\
	bool _ok = true;								\
	for (size_t _i = gGpuActiveLayers.validationLayerCount; _i > 0; _i--) {		\
		auto function = gGpuActiveLayers.validationLayers[_i - 1]->_function;	\
		if (function != nullptr) {						\
			_ok = function();						\
			if (!_ok) {							\
				break;							\
			}								\
		}									\
	}										\
	if (_ok && gGpuActiveLayers.baseLayer != nullptr) {				\
		auto function = gGpuActiveLayers.baseLayer->_function;			\
		if (function != nullptr) {						\
			return function();						\
		}									\
	}										\
} while(false);

#define GPU_LAYERED_CALL_NO_PARAMS_NO_RETURN(_function)					\
do {											\
	bool _ok = true;								\
	for (size_t _i = gGpuActiveLayers.validationLayerCount; _i > 0; _i--) {		\
		auto function = gGpuActiveLayers.validationLayers[_i - 1]->_function;	\
		if (function != nullptr) {						\
			_ok = function();						\
			if (!_ok) {							\
				break;							\
			}								\
		}									\
	}										\
	if (_ok && gGpuActiveLayers.baseLayer != nullptr) {				\
		auto function = gGpuActiveLayers.baseLayer->_function;			\
		if (function != nullptr) {						\
			function();							\
		}									\
	}										\
} while(false);

#define GPU_LAYERED_CALL(_function, ...)						\
do {											\
	bool _ok = true;								\
	for (size_t _i = gGpuActiveLayers.validationLayerCount; _i > 0; _i--) {		\
		auto function = gGpuActiveLayers.validationLayers[_i - 1]->_function;	\
		if (function != nullptr) {						\
			_ok = function(__VA_ARGS__);					\
			if (!_ok) {							\
				break;							\
			}								\
		}									\
	}										\
	if (_ok && gGpuActiveLayers.baseLayer != nullptr) {				\
		auto function = gGpuActiveLayers.baseLayer->_function;			\
		if (function != nullptr) {						\
			return function(__VA_ARGS__);					\
		}									\
	}										\
} while(false);

bool gpuPushLayer(const GpuLayer* layer);

// NOTE: Platform specific
GpuBaseLayer* gpuAcquireBaseLayerFor(GpuBackend backend);

// NOTE: Platform specific
GpuLayer* gpuAcquireValidationLayerFor(GpuBackend backend);

#endif // GPU_LAYERS_H

