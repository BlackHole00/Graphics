#include "textures.h"

#include "allocation.h"

Mtl4TextureStorage gMtl4TextureStorage;

MTLPixelFormat gMtl4GpuToMtlFormat[] = {
	/*GPU_FORMAT_NONE=*/			MTLPixelFormatInvalid,
	/*GPU_FORMAT_R8_UNORM=*/		MTLPixelFormatR8Unorm,
	/*GPU_FORMAT_RG8_UNORM=*/		MTLPixelFormatRG8Unorm,
	/*GPU_FORMAT_RGBA8_UNORM=*/		MTLPixelFormatRGBA8Unorm,
	/*GPU_FORMAT_RGBA8_SRGB=*/		MTLPixelFormatRGBA8Unorm_sRGB,
	/*GPU_FORMAT_BGRA8_UNORM=*/		MTLPixelFormatBGRA8Unorm,
	/*GPU_FORMAT_BGRA8_SRGB=*/		MTLPixelFormatBGRA8Unorm_sRGB,
	/*GPU_FORMAT_R16_FLOAT=*/		MTLPixelFormatR16Float,
	/*GPU_FORMAT_RG16_FLOAT=*/		MTLPixelFormatRG16Float,
	/*GPU_FORMAT_RGBA16_FLOAT=*/		MTLPixelFormatRGBA16Float,
	/*GPU_FORMAT_RGBA16_UNORM=*/		MTLPixelFormatRGBA16Unorm,
	/*GPU_FORMAT_R16_UNORM=*/		MTLPixelFormatR16Unorm,
	/*GPU_FORMAT_RG16_UNORM=*/		MTLPixelFormatRG16Unorm,
	/*GPU_FORMAT_R32_FLOAT=*/		MTLPixelFormatR32Float,
	/*GPU_FORMAT_RG32_FLOAT=*/		MTLPixelFormatRG32Float,
	/*GPU_FORMAT_RGBA32_FLOAT=*/		MTLPixelFormatRGBA32Float,
	/*GPU_FORMAT_RG11B10_FLOAT=*/		MTLPixelFormatRG11B10Float,
	/*GPU_FORMAT_RGB10_A2_UNORM=*/		MTLPixelFormatRGB10A2Unorm,
	/*GPU_FORMAT_RGB10_A2_UINT=*/		MTLPixelFormatRGB10A2Uint,
	/*GPU_FORMAT_D32_FLOAT=*/		MTLPixelFormatDepth32Float,
	/*GPU_FORMAT_D24_UNORM_S8_UINT=*/	MTLPixelFormatDepth24Unorm_Stencil8,
	/*GPU_FORMAT_D32_FLOAT_S8_UINT=*/	MTLPixelFormatDepth32Float_Stencil8,
	/*GPU_FORMAT_D16_UNORM=*/		MTLPixelFormatDepth16Unorm,
	/*GPU_FORMAT_BC1_RGBA_UNORM=*/		MTLPixelFormatBC1_RGBA,
	/*GPU_FORMAT_BC1_RGBA_SRGB=*/		MTLPixelFormatBC1_RGBA_sRGB,
	/*GPU_FORMAT_BC4_UNORM=*/		MTLPixelFormatBC4_RUnorm,
	/*GPU_FORMAT_BC5_UNORM=*/		MTLPixelFormatBC5_RGUnorm,
};

MTLTextureType gMtl4GpuToMtlTextureType[] = {
	/*GPU_TEXTURE_1D=*/		MTLTextureType1D,
	/*GPU_TEXTURE_2D=*/		MTLTextureType2D,
	/*GPU_TEXTURE_3D=*/		MTLTextureType3D,
	/*GPU_TEXTURE_CUBE=*/		MTLTextureTypeCube,
	/*GPU_TEXTURE_2D_ARRAY=*/	MTLTextureType2DArray,
	/*GPU_TEXTURE_CUBE_ARRAY=*/	MTLTextureTypeCubeArray,
};

MTLTextureUsage gMtl4GpuToMtlUsage[] = {
	/*GPU_USAGE_SAMPLED*/	 		MTLTextureUsageShaderRead,
	/*GPU_USAGE_STORAGE*/			MTLTextureUsageShaderWrite | MTLTextureUsageShaderWrite,
	/*GPU_USAGE_COLOR_ATTACHMENT*/		MTLTextureUsageRenderTarget,
	/*GPU_USAGE_DEPTH_STENCIL_ATTACHMENT*/	MTLTextureUsageRenderTarget,
};

size_t gMtl4GpuFormatPixelSize[] = {
	/*GPU_FORMAT_NONE=*/			0,
	/*GPU_FORMAT_R8_UNORM=*/		1,
	/*GPU_FORMAT_RG8_UNORM=*/		2,
	/*GPU_FORMAT_RGBA8_UNORM=*/		4,
	/*GPU_FORMAT_RGBA8_SRGB=*/		4,
	/*GPU_FORMAT_BGRA8_UNORM=*/		4,
	/*GPU_FORMAT_BGRA8_SRGB=*/		4,
	/*GPU_FORMAT_R16_FLOAT=*/		2,
	/*GPU_FORMAT_RG16_FLOAT=*/		4,
	/*GPU_FORMAT_RGBA16_FLOAT=*/		8,
	/*GPU_FORMAT_RGBA16_UNORM=*/		8,
	/*GPU_FORMAT_R16_UNORM=*/		2,
	/*GPU_FORMAT_RG16_UNORM=*/		4,
	/*GPU_FORMAT_R32_FLOAT=*/		4,
	/*GPU_FORMAT_RG32_FLOAT=*/		8,
	/*GPU_FORMAT_RGBA32_FLOAT=*/		16,
	/*GPU_FORMAT_RG11B10_FLOAT=*/		4,
	/*GPU_FORMAT_RGB10_A2_UNORM=*/		4,
	/*GPU_FORMAT_RGB10_A2_UINT=*/		4,
	/*GPU_FORMAT_D32_FLOAT=*/		4,
	/*GPU_FORMAT_D24_UNORM_S8_UINT=*/	4,
	/*GPU_FORMAT_D32_FLOAT_S8_UINT=*/	8,
	/*GPU_FORMAT_D16_UNORM=*/		2,
	/*GPU_FORMAT_BC1_RGBA_UNORM=*/		8,	// Block size for 4x4 pixels
	/*GPU_FORMAT_BC1_RGBA_SRGB=*/		8,	// Block size for 4x4 pixels
	/*GPU_FORMAT_BC4_UNORM=*/		8,	// Block size for 4x4 pixels
	/*GPU_FORMAT_BC5_UNORM=*/		16,	// Block size for 4x4 pixels
};

void mtl4InitTextureStorage(GpuResult* result) {
	CmnResult localResult;

	gMtl4TextureStorage.textureMetadataPage = cmnCreatePage(4 * 1024 * 1024, CMN_PAGE_READABLE | CMN_PAGE_WRITABLE, &localResult);
	if (localResult != CMN_SUCCESS) {
		CMN_SET_RESULT(result, GPU_OUT_OF_CPU_MEMORY);
		goto on_error_cleanup;
	}

	gMtl4TextureStorage.textureMedatadaArena = cmnPageToArena(gMtl4TextureStorage.textureMetadataPage);

	CmnAllocator allocator;
	allocator = cmnArenaAllocator(&gMtl4TextureStorage.textureMedatadaArena);

	cmnCreateHandleMap(&gMtl4TextureStorage.textures, allocator, {}, &localResult);
	if (localResult != CMN_SUCCESS) {
		CMN_SET_RESULT(result, GPU_OUT_OF_CPU_MEMORY);
		goto on_error_cleanup;
	}

	CMN_SET_RESULT(result, GPU_SUCCESS);
	return;

on_error_cleanup:
	mtl4FiniTextureStorage();
}

void mtl4FiniTextureStorage(void) {
	cmnDestroyPage(gMtl4TextureStorage.textureMetadataPage);

	gMtl4TextureStorage = {};
}

GpuTexture mtl4CreateTexture(const GpuTextureDesc* desc, void* ptrGpu, GpuResult* result) {
	CmnResult localResult;

	// TODO: Implement range based lookup also for gpu memory;
	id<MTLBuffer> referenceBuffer;
	size_t offsetInBuffer;

	{
		CmnScopedMutex guard(&gMtl4AllocationStorage.allocationMutex);

		Mtl4AllocationMetadata* metadata = mtl4GetAllocationMetadataOf(ptrGpu, true);
		if (metadata == nullptr) {
			CMN_SET_RESULT(result, GPU_NO_SUCH_ALLOCATION_FOUND);
			return 0;
		}

		if (
			((uintptr_t)ptrGpu < (uintptr_t)metadata->gpuAddress) ||
			((uintptr_t)ptrGpu >= (uintptr_t)metadata->gpuAddress + metadata->size)
		) {
			CMN_SET_RESULT(result, GPU_ALLOCATION_MEMORY_IS_CPU);
			return 0;
		}

		offsetInBuffer = (uintptr_t)ptrGpu - (uintptr_t)metadata->gpuAddress;
		referenceBuffer = metadata->buffer;
	}

	MTLTextureDescriptor* textureDescriptor = [MTLTextureDescriptor new];
	textureDescriptor.textureType		= gMtl4GpuToMtlTextureType[desc->type];
	textureDescriptor.pixelFormat		= gMtl4GpuToMtlFormat[desc->format];
	textureDescriptor.width			= desc->dimensions[0];
	textureDescriptor.height		= desc->dimensions[1];
	textureDescriptor.mipmapLevelCount 	= desc->mipCount;
	textureDescriptor.sampleCount		= desc->sampleCount;
	textureDescriptor.resourceOptions 	= referenceBuffer.resourceOptions;
	textureDescriptor.usage			= gMtl4GpuToMtlUsage[desc->usage];
	// TODO: Maybe not always true.
	textureDescriptor.allowGPUOptimizedContents = true;
	// TODO: Some formats should be compressed
	textureDescriptor.compressionType 	= MTLTextureCompressionTypeLossless;
	textureDescriptor.swizzle 		= MTLTextureSwizzleChannelsDefault;
	textureDescriptor.placementSparsePageSize = (MTLSparsePageSize)0;

	if (desc->type == GPU_TEXTURE_3D) {
		textureDescriptor.depth	= desc->dimensions[2];
	} else {
		textureDescriptor.depth	= 1;
	}

	if (desc->type == GPU_TEXTURE_2D_ARRAY || desc->type == GPU_TEXTURE_CUBE_ARRAY) {
		textureDescriptor.arrayLength	= desc->dimensions[2];
	} else {
		textureDescriptor.arrayLength	= 1;
	}

	id<MTLTexture> texture = [referenceBuffer newTextureWithDescriptor:textureDescriptor
		offset:offsetInBuffer
		bytesPerRow:gMtl4GpuFormatPixelSize[desc->format] * desc->dimensions[0]];
	if (texture == nil) {
		CMN_SET_RESULT(result, GPU_OUT_OF_GPU_MEMORY);
		return 0;
	}

	Mtl4TextureMetadata metadata;
	metadata.texture = texture;
	metadata.descriptor.data[0] = [texture gpuResourceID]._impl;

	CmnHandle handle;

	{
		CmnScopedMutex guard(&gMtl4TextureStorage.mutex);

		handle = cmnInsert(&gMtl4TextureStorage.textures, metadata, &localResult);
		if (localResult != CMN_SUCCESS) {
			CMN_SET_RESULT(result, GPU_OUT_OF_CPU_MEMORY);
			goto on_error_cleanup;
		}
	}

	[textureDescriptor release];
	return *(GpuTexture*)&handle;

on_error_cleanup:
	[texture release];
	[textureDescriptor release];
	
	return 0;
}

