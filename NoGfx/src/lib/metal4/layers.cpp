#include "layers.h"

#include <lib/metal4/context.h>
#include <lib/metal4/device.h>

GpuLayer gMtl4BaseLayer = {
	/*layerInit=*/			mtl4Init,
	/*gpuDeinit=*/			nullptr,
	/*gpuEnumerateDevices=*/	mtl4EnumerateDevices,
	/*gpuSelectDevice=*/		nullptr,
};

GpuLayer gMtl4ValidationLayer;

