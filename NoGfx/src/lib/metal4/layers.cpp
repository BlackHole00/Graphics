#include "layers.h"

#include <lib/metal4/context.h>
#include <lib/metal4/device.h>
#include <lib/metal4/allocation.h>
#include <lib/metal4/textures.h>
#include <lib/metal4/validation.h>

GpuBaseLayer gMtl4BaseLayer = {
	/*layerInit=*/			mtl4Init,
	/*gpuDeinit=*/			mtl4Deinit,
	/*gpuEnumerateDevices=*/	mtl4EnumerateDevices,
	/*gpuSelectDevice=*/		mtl4SelectDevice,
	/*gpuMalloc=*/			mtl4Malloc,
	/*gpuFree=*/			mtl4Free,
	/*gpuHostToDevicePointer=*/	mtl4HostToDevicePointer,
	/*gpuTextureSizeAlign=*/	mtl4TextureSizeAlign,
	/*gpuCreateTexture=*/		mtl4CreateTexture,
	/*gpuTextureViewDescriptor=*/	nullptr,
	/*gpuRWTextureViewDescriptor=*/	nullptr,
};

GpuLayer gMtl4ValidationLayer = {
	/*layerInit=*/			nullptr,
	/*gpuDeinit=*/			nullptr,
	/*gpuEnumerateDevices=*/	mtl4ValidateEnumerateDevices,
	/*gpuSelectDevice=*/		mtl4ValidateSelectDevice,
	/*gpuMalloc=*/			mtl4ValidateGpuMalloc,
	/*gpuFree=*/ 			nullptr,
	/*gpuHostToDevicePointer=*/	mtl4ValidateGpuHostToDevicePointer,
	/*gpuTextureSizeAlign=*/	nullptr,
	/*gpuCreateTexture=*/		nullptr,
	/*gpuTextureViewDescriptor=*/	nullptr,
	/*gpuRWTextureViewDescriptor=*/	nullptr,
};

