#include "layers.h"

#include <lib/metal4/context.h>
#include <lib/metal4/device.h>
#include <lib/metal4/allocation.h>
#include <lib/metal4/textures.h>
#include <lib/metal4/pipelines.h>
#include <lib/metal4/queue.h>
#include <lib/metal4/command_buffers.h>
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
	/*gpuTextureViewDescriptor=*/	mtl4TextureViewDescriptor,
	/*gpuRWTextureViewDescriptor=*/	mtl4RWTextureViewDescriptor,
	/*gpuCreateComputePipeline=*/	mtl4CreateComputePipeline,
	/*gpuCreateRenderPipeline=*/	mtl4CreateRenderPipeline,
	/*gpuCreateMeshletPipeline=*/	mtl4CreateMeshletPipeline,
	/*gpuFreePipeline=*/		mtl4FreePipeline,
	/*gpuCreateQueue=*/		mtl4CreateQueue,
	/*gpuStartCommandEncoding=*/	mtl4StartCommandEncoding,
	/*gpuSubmit=*/			mtl4Submit,
	/*gpuSubmitWithSignal=*/	mtl4SubmitWithSignal,
	/*gpuMemCpy=*/			mtl4MemCpy,
	/*gpuCopyToTexture=*/		mtl4CopyToTexture,
	/*gpuCopyFromTexture=*/		mtl4CopyFromTexture,
	/*gpuBarrier=*/			mtl4Barrier,
};

GpuLayer gMtl4ValidationLayer = {
	/*layerInit=*/			nullptr,
	/*gpuDeinit=*/			nullptr,
	/*gpuEnumerateDevices=*/	mtl4ValidateEnumerateDevices,
	/*gpuSelectDevice=*/		mtl4ValidateSelectDevice,
	/*gpuMalloc=*/			mtl4ValidateGpuMalloc,
	/*gpuFree=*/ 			nullptr,
	/*gpuHostToDevicePointer=*/	mtl4ValidateGpuHostToDevicePointer,
	/*gpuTextureSizeAlign=*/	mtl4ValidateGpuTextureSizeAndAlign,
	/*gpuCreateTexture=*/		mtl4ValidateGpuCreateTexture,
	/*gpuTextureViewDescriptor=*/	mtl4ValidateGpuTextureViewDescriptor,
	/*gpuRWTextureViewDescriptor=*/	mtl4ValidateGpuTextureRWViewDescriptor,
	/*gpuCreateComputePipeline=*/	nullptr,
	/*gpuCreateRenderPipeline=*/	nullptr,
	/*gpuCreateMeshletPipeline=*/	nullptr,
	/*gpuFreePipeline=*/		nullptr,
	/*gpuCreateQueue=*/		nullptr,
	/*gpuStartCommandEncoding=*/	nullptr,
	/*gpuSubmit=*/			nullptr,
	/*gpuSubmitWithSignal=*/	nullptr,
	/*gpuMemCpy=*/			nullptr,
	/*gpuCopyToTexture=*/		nullptr,
	/*gpuCopyFromTexture=*/		nullptr,
	/*gpuBarrier=*/			nullptr,
};

