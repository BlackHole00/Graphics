#include "layers.h"

#ifdef CMN_PLATFORM_DARWIN

#include <lib/metal4/layers.h>

GpuBaseLayer* gpuAcquireBaseLayerFor(GpuBackend backend) {
	switch (backend) {
		case GPU_METAL_4: {
			return &gMtl4BaseLayer;
		}
		default: {
			return nullptr;
		}
	}
}

GpuLayer* gpuAcquireValidationLayerFor(GpuBackend backend) {
	switch (backend) {
		case GPU_METAL_4: {
			return &gMtl4ValidationLayer;
		}
		default: {
			return nullptr;
		}
	}
}


#endif // GPU_PLATFORM_DARWIN

