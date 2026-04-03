#include "layers.h"

#include <lib/metal4/context.h>
#include <lib/metal4/device.h>
#include <lib/metal4/validation.h>

GpuLayer gMtl4BaseLayer = {
	/*layerInit=*/			mtl4Init,
	/*gpuDeinit=*/			mtl4Deinit,
	/*gpuEnumerateDevices=*/	mtl4EnumerateDevices,
	/*gpuSelectDevice=*/		mtl4SelectDevice,
};

GpuLayer gMtl4ValidationLayer = {
	/*layerInit=*/			nullptr,
	/*gpuDeinit=*/			nullptr,
	/*gpuEnumerateDevices=*/	mtl4ValidateEnumerateDevices,
	/*gpuSelectDevice=*/		mtl4ValidateSelectDevice,
};

