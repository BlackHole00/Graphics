#include "textures.h"

#include <lib/metal4/context.h>
#include <lib/metal4/allocation.h>

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
	/*GPU_USAGE_STORAGE*/			MTLTextureUsageShaderRead | MTLTextureUsageShaderWrite,
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

	gMtl4TextureStorage.textureViewsPage = cmnCreatePage(4 * 1024 * 1024, CMN_PAGE_READABLE | CMN_PAGE_WRITABLE, &localResult);
	if (localResult != CMN_SUCCESS) {
		CMN_SET_RESULT(result, GPU_OUT_OF_CPU_MEMORY);
		goto on_error_cleanup;
	}

	gMtl4TextureStorage.textureMedatadaArena = cmnPageToArena(gMtl4TextureStorage.textureMetadataPage);
	gMtl4TextureStorage.textureViewsPool = cmnPageToPool(gMtl4TextureStorage.textureViewsPage, sizeof(Mtl4TextureViews));

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
	cmnDestroyPage(gMtl4TextureStorage.textureViewsPage);

	gMtl4TextureStorage = {};
}

GpuTexture mtl4CreateTexture(const GpuTextureDesc* desc, void* ptrGpu, GpuResult* result) {
	CmnResult localResult;
	GpuResult localGpuResult;

	if (mtl4IsCpuAddress(ptrGpu)) {
		return GPU_ALLOCATION_MEMORY_IS_CPU;
	}


	size_t backingSize = 0;
	size_t backingAlign = 0;
	id<MTLHeap> backingHeap = nil;

	{
		CmnScopedMutex guard(&gMtl4AllocationStorage.mutex);
		
		Mtl4AllocationMetadata* metadata = mtl4GetAllocationMetadataOf(ptrGpu, true);
		if (metadata == nullptr) {
			CMN_SET_RESULT(result, GPU_NO_SUCH_ALLOCATION_FOUND);
			return 0;
		}

		if (metadata->memory != GPU_MEMORY_GPU) {
			CMN_SET_RESULT(result, GPU_ALLOCATION_MEMORY_IS_CPU);
			return 0;
		}

		backingSize	= metadata->size;
		backingAlign	= metadata->align;
		backingHeap	= metadata->associatedTextureHeap;

		if (backingHeap != nil) {
			[backingHeap retain];
		}
	}

	MTLTextureDescriptor* textureDescriptor = mtl4GpuTextureDescToMtl(
		desc,
		MTLResourceStorageModePrivate | MTLResourceHazardTrackingModeUntracked
	);
	id<MTLTexture> texture;

	if (backingHeap == nil) {
		
		GpuTextureSizeAlign expectedSizeAlign = mtl4TextureSizeAlign(desc, nullptr);
		if (expectedSizeAlign.align == backingAlign && expectedSizeAlign.size == backingSize) {
			// The buffer has been made to contain only the texture;

			texture = [gMtl4Context.device newTextureWithDescriptor:textureDescriptor];
			if (texture == nil) {
				CMN_SET_RESULT(result, GPU_OUT_OF_GPU_MEMORY);
				return 0;
			}
		} else if (expectedSizeAlign.align <= backingAlign && expectedSizeAlign.size < backingSize) {
			// The buffer must contain a new heap, for multiple textures
			MTLHeapDescriptor* heapDescriptor = [MTLHeapDescriptor new];
			heapDescriptor.resourceOptions = MTLResourceStorageModePrivate | MTLResourceHazardTrackingModeUntracked;
			heapDescriptor.size = backingSize;

			backingHeap = [gMtl4Context.device newHeapWithDescriptor:heapDescriptor];
			if (backingHeap == nil) {
				CMN_SET_RESULT(result, GPU_OUT_OF_GPU_MEMORY);
				return 0;
			}

			texture = [backingHeap newTextureWithDescriptor:textureDescriptor];
			if (texture == nil) {
				[backingHeap release];

				CMN_SET_RESULT(result, GPU_OUT_OF_GPU_MEMORY);
				return 0;
			}

			{
				CmnScopedMutex guard(&gMtl4AllocationStorage.mutex);
	
				Mtl4AllocationMetadata* metadata = mtl4GetAllocationMetadataOf(ptrGpu, true);
				if (metadata == nullptr) {
					[texture release];
					[backingHeap release];

					CMN_SET_RESULT(result, GPU_NO_SUCH_ALLOCATION_FOUND);
					return 0;
				}

				metadata->associatedTextureHeap = [backingHeap retain];
			}
		} else {
			// The buffer is not big enough

			CMN_SET_RESULT(result, GPU_OUT_OF_GPU_MEMORY);
			return 0;
		}

	} else {
		texture = [backingHeap newTextureWithDescriptor:textureDescriptor];
		if (texture == nil) {
			CMN_SET_RESULT(result, GPU_OUT_OF_GPU_MEMORY);
			return 0;
		}
	}

	Mtl4TextureMetadata textureMetadata;
	textureMetadata.texture = texture;
	memcpy(&textureMetadata.descriptor, desc, sizeof(GpuTextureDesc));

	Mtl4Texture textureId;

	{
		CmnScopedMutex guard(&gMtl4TextureStorage.mutex);

		textureId = cmnInsert(&gMtl4TextureStorage.textures, textureMetadata, &localResult);
		if (localResult != CMN_SUCCESS) {
			[texture release];

			CMN_SET_RESULT(result, GPU_OUT_OF_CPU_MEMORY);
			return 0;
		}
	}

	{
		CmnScopedMutex guard(&gMtl4AllocationStorage.mutex);

		Mtl4AllocationMetadata* metadata = mtl4GetAllocationMetadataOf(ptrGpu, true);
		if (metadata == nullptr) {
			CMN_SET_RESULT(result, GPU_NO_SUCH_ALLOCATION_FOUND);
			return 0;
		}

		mtl4AssociateTextureToAllocation(metadata, textureId, &localGpuResult);
		if (localGpuResult != GPU_SUCCESS) {
			[texture release];
			cmnRemove(&gMtl4TextureStorage.textures, textureId);
			
			CMN_SET_RESULT(result, localGpuResult);
			return 0;
		}
	}

	if (backingHeap != nil) {
		[backingHeap release];
	}

	[textureDescriptor release];

	CMN_SET_RESULT(result, GPU_SUCCESS);
	return *(GpuTexture*)&textureId;
}

GpuTextureSizeAlign mtl4TextureSizeAlign(const GpuTextureDesc* desc, GpuResult* result) {
	(void)result;

	MTLTextureDescriptor* textureDescriptor = mtl4GpuTextureDescToMtl(
		desc,
		MTLResourceStorageModePrivate | MTLResourceHazardTrackingModeUntracked
	);

	MTLSizeAndAlign sizeNAlign = [gMtl4Context.device heapTextureSizeAndAlignWithDescriptor:textureDescriptor];

	[textureDescriptor release];
	return {
		/*size=*/	sizeNAlign.size,
		/*align=*/	sizeNAlign.align,
	};
}

GpuTextureDescriptor mtl4TextureViewDescriptor(GpuTexture texture, const GpuViewDesc* desc, GpuResult* result) {
	return {};
}
GpuTextureDescriptor mtl4RWTextureViewDescriptor(GpuTexture texture, const GpuViewDesc* desc, GpuResult* result) {
	return {};
}

void mtl4DestroyTexture(Mtl4Texture texture) {
}

MTLTextureDescriptor* mtl4GpuTextureDescToMtl(const GpuTextureDesc* desc, MTLResourceOptions resourceOptions) {
	MTLTextureDescriptor* textureDescriptor = [MTLTextureDescriptor new];

	textureDescriptor.textureType		= gMtl4GpuToMtlTextureType[desc->type];
	textureDescriptor.pixelFormat		= gMtl4GpuToMtlFormat[desc->format];
	textureDescriptor.width			= desc->dimensions[0];
	textureDescriptor.height		= (desc->type == GPU_TEXTURE_1D) ? 1 : desc->dimensions[1];
	textureDescriptor.mipmapLevelCount 	= desc->mipCount;
	textureDescriptor.sampleCount		= desc->sampleCount;
	textureDescriptor.resourceOptions 	= resourceOptions;
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
		textureDescriptor.arrayLength	= desc->layerCount;
	} else {
		textureDescriptor.arrayLength	= 1;
	}

	return textureDescriptor;
}

MTLTextureViewDescriptor* mtl4GpuViewDescToMtl(id<MTLTexture> referenceTexture, const GpuViewDesc* desc) {
	MTLTextureViewDescriptor* viewDescriptor = [MTLTextureViewDescriptor new];

	viewDescriptor.pixelFormat	= gMtl4GpuToMtlFormat[desc->format];
	viewDescriptor.textureType	= referenceTexture.textureType;
	viewDescriptor.levelRange	= NSMakeRange(desc->baseMip, desc->mipCount);
	viewDescriptor.sliceRange	= NSMakeRange(desc->baseLayer, desc->layerCount);
	viewDescriptor.swizzle		= MTLTextureSwizzleChannelsDefault;

	return viewDescriptor;
}

