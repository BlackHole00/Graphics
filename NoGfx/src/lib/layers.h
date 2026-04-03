#ifndef GPU_LAYERS_H
#define GPU_LAYERS_H

#include <gpu/gpu.h>
#include <lib/common/common.h>

#define GPU_MAX_LAYERS 4

typedef struct {
	const GpuLayer*	layers[GPU_MAX_LAYERS];
	size_t		count;
} GpuActiveLayers;
extern GpuActiveLayers gGpuActiveLayers;

#define GPU_LAYERED_CALL_NO_RESULT(_function)				\
for (size_t i = gGpuActiveLayers.count; i > 0; i--) {			\
	auto function = gGpuActiveLayers.layers[i - 1]->_function;	\
	if (function != nullptr) {					\
		function();						\
	}								\
}

#define GPU_LAYERED_CALL_NO_PARAMS(_function, _result_ptr)		\
for (size_t i = gGpuActiveLayers.count; i > 0; i--) {			\
	auto function = gGpuActiveLayers.layers[i - 1]->_function;	\
	if (function != nullptr) {					\
		function(_result_ptr);					\
		if (*_result_ptr != GPU_SUCCESS) {			\
			break;						\
		}							\
	}								\
}

#define GPU_LAYERED_CALL(_function, _result_ptr, ...)			\
for (size_t i = gGpuActiveLayers.count; i > 0; i--) {			\
	auto function = gGpuActiveLayers.layers[i - 1]->_function;	\
	if (function != nullptr) {					\
		function(__VA_ARGS__, _result_ptr);			\
		if (*_result_ptr != GPU_SUCCESS) {			\
			break;						\
		}							\
	}								\
}

bool gpuPushLayer(const GpuLayer* layer);

// NOTE: Platform specific
GpuLayer* gpuAcquireBaseLayerFor(GpuBackend backend);

// NOTE: Platform specific
GpuLayer* gpuAcquireValidationLayerFor(GpuBackend backend);

#endif // GPU_LAYERS_H

