#include "layers.h"

#include <lib/metal4/context.h>
#include <lib/metal4/device.h>
#include <lib/metal4/allocation.h>
#include <lib/metal4/validation.h>

GpuBaseLayer gMtl4BaseLayer = {
	/*layerInit=*/			mtl4Init,
	/*gpuDeinit=*/			mtl4Deinit,
	/*gpuEnumerateDevices=*/	mtl4EnumerateDevices,
	/*gpuSelectDevice=*/		mtl4SelectDevice,
	/*gpuMalloc=*/			mtl4Malloc,
	/*gpuFree=*/ 			mtl4Free,
	/*gpuHostToDevicePointer=*/	mtl4HostToDevicePointer,
};

GpuLayer gMtl4ValidationLayer = {
	/*layerInit=*/			nullptr,
	/*gpuDeinit=*/			nullptr,
	/*gpuEnumerateDevices=*/	mtl4ValidateEnumerateDevices,
	/*gpuSelectDevice=*/		mtl4ValidateSelectDevice,
	/*gpuMalloc=*/			nullptr,
	/*gpuFree=*/ 			nullptr,
	/*gpuHostToDevicePointer=*/	nullptr,
};

