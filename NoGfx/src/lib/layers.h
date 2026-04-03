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
for (size_t _i = gGpuActiveLayers.count; _i > 0; _i--) {		\
	auto function = gGpuActiveLayers.layers[_i - 1]->_function;	\
	if (function != nullptr) {					\
		function();						\
	}								\
}

#define GPU_LAYERED_CALL_NO_PARAMS(_function, _result_ptr)			\
do {										\
	GpuResult _result;							\
	for (size_t _i = gGpuActiveLayers.count; _i > 0; _i--) {		\
		auto function = gGpuActiveLayers.layers[_i - 1]->_function;	\
		if (function != nullptr) {					\
			function(&_result);					\
			if (_result != GPU_SUCCESS) {				\
				break;						\
			}							\
		}								\
	}									\
	if (_result_ptr != nullptr) {						\
		*_result_ptr = _result;						\
	}									\
} while(0);

#define GPU_LAYERED_CALL(_function, _result_ptr, ...)				\
do {										\
	GpuResult _result;							\
	for (size_t _i = gGpuActiveLayers.count; _i > 0; _i--) {		\
		auto function = gGpuActiveLayers.layers[_i - 1]->_function;	\
		if (function != nullptr) {					\
			function(__VA_ARGS__, &_result);			\
			if (_result != GPU_SUCCESS) {				\
				break;						\
			}							\
		}								\
	}									\
	if (_result_ptr != nullptr) {						\
		*_result_ptr = _result;						\
	}									\
} while(0);

bool gpuPushLayer(const GpuLayer* layer);

// NOTE: Platform specific
GpuLayer* gpuAcquireBaseLayerFor(GpuBackend backend);

// NOTE: Platform specific
GpuLayer* gpuAcquireValidationLayerFor(GpuBackend backend);

#endif // GPU_LAYERS_H

