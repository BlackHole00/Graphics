#include "layers.h"

GpuActiveLayers gGpuActiveLayers;

bool gpuPushLayer(const GpuLayer* layer) {
	if (gGpuActiveLayers.count >= GPU_MAX_LAYERS) {
		return false;
	}

	gGpuActiveLayers.layers[gGpuActiveLayers.count] = layer;
	gGpuActiveLayers.count++;

	return true;
}

