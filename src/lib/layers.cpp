#include "layers.h"
#include <atomic>

GpuActiveLayers gGpuActiveLayers;

bool gpuPushLayer(const GpuLayer* layer) {
	if (gGpuActiveLayers.validationLayerCount >= GPU_MAX_LAYERS) {
		return false;
	}

	gGpuActiveLayers.validationLayers[gGpuActiveLayers.validationLayerCount] = layer;
	gGpuActiveLayers.validationLayerCount++;

	return true;
}

