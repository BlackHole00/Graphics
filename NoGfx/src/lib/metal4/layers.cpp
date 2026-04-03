#include "layers.h"

#include <lib/metal4/context.h>
#include <lib/metal4/device.h>

GpuLayer gMtl4BaseLayer = {
	/*layerInit=*/			mtl4Init,
	/*gpuDeinit=*/			mtl4Deinit,
	/*gpuEnumerateDevices=*/	mtl4EnumerateDevices,
	/*gpuSelectDevice=*/		mtl4SelectDevice,
};

GpuLayer gMtl4ValidationLayer;

