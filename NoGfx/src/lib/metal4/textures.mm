#include "textures.h"

#include <lib/common/memory.h>
#include <lib/metal4/context.h>
#include <lib/metal4/allocation.h>
#include <lib/metal4/deletion_manager.h>

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

	size_t offsetFromBase = mtl4GpuAddressOffsetFromBase(ptrGpu);

	Mtl4AllocationMetadata* metadata = mtl4AcquireAllocationMetadataFromGpuPtr(ptrGpu);
	if (metadata == nullptr) {
		CMN_SET_RESULT(result, GPU_NO_SUCH_ALLOCATION_FOUND);
		return 0;
	}
	defer (mtl4ReleaseAllocationMetadata());
	
	MTLTextureDescriptor* textureDescriptor = mtl4GpuTextureDescToMtl(
		desc,
		MTLResourceStorageModePrivate | MTLResourceHazardTrackingModeUntracked
	);
	defer ([textureDescriptor release]);

	id<MTLTexture> texture;
	id<MTLHeap> backingHeap = cmnAtomicLoad(&metadata->associatedTextureHeap);

	if (backingHeap == nil) {
		GpuTextureSizeAlign expectedSizeAlign = mtl4TextureSizeAlign(desc, nullptr);

		if (offsetFromBase == 0 && expectedSizeAlign.size == metadata->size) {
			// The buffer has been made to contain only the texture;

			texture = [gMtl4Context.device newTextureWithDescriptor:textureDescriptor];
			if (texture == nil) {
				CMN_SET_RESULT(result, GPU_OUT_OF_GPU_MEMORY);
				return 0;
			}
		} else if (
			cmnIsAlignedTo((uintptr_t)ptrGpu, expectedSizeAlign.align) &&
			(metadata->size - offsetFromBase) >= expectedSizeAlign.size
		) {
			// The buffer must contain a new heap, for multiple textures
			MTLHeapDescriptor* heapDescriptor = [MTLHeapDescriptor new];
			defer ([heapDescriptor release]);

			heapDescriptor.resourceOptions = MTLResourceStorageModePrivate | MTLResourceHazardTrackingModeUntracked;
			heapDescriptor.size = metadata->size;

			backingHeap = [gMtl4Context.device newHeapWithDescriptor:heapDescriptor];
			if (backingHeap == nil) {
				CMN_SET_RESULT(result, GPU_OUT_OF_GPU_MEMORY);
				return 0;
			}

			// NOTE: Another thread may have got here before us. If so, let's use the heap set by the other
			//	thread.
			if (!cmnAtomicCompareExchangeStrong(&metadata->associatedTextureHeap, (id<MTLHeap>)nil, backingHeap)) {
				[backingHeap release];
				backingHeap = cmnAtomicLoad(&metadata->associatedTextureHeap);
			}

			texture = [backingHeap newTextureWithDescriptor:textureDescriptor];
			if (texture == nil) {
				[backingHeap release];

				CMN_SET_RESULT(result, GPU_OUT_OF_GPU_MEMORY);
				return 0;
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

	Mtl4TextureMetadata textureMetadata = {};
	textureMetadata.texture = texture;
	memcpy(&textureMetadata.descriptor, desc, sizeof(GpuTextureDesc));

	Mtl4Texture textureHandle;
	{
		CmnScopedStorageSyncLockWrite guard(&gMtl4TextureStorage.sync);

		textureHandle = cmnInsert(&gMtl4TextureStorage.textures, textureMetadata, &localResult);
		if (localResult != CMN_SUCCESS) {
			[texture release];

			CMN_SET_RESULT(result, GPU_OUT_OF_CPU_MEMORY);
			return 0;
		}
	}

	mtl4AssociateTextureToAllocation(metadata, textureHandle, &localGpuResult);
	if (localGpuResult != GPU_SUCCESS) {
		[texture release];
		{
			CmnScopedStorageSyncLockWrite guard(&gMtl4TextureStorage.sync);
			cmnRemove(&gMtl4TextureStorage.textures, textureHandle);
		}
		
		CMN_SET_RESULT(result, localGpuResult);
		return 0;
	}

	CMN_SET_RESULT(result, GPU_SUCCESS);
	return mtl4HandleToGpuTexture(textureHandle);
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

Mtl4TextureMetadata* mtl4AcquireTextureMetadataFrom(Mtl4Texture texture) {
	bool wasHandleValid;
	Mtl4TextureMetadata* metadata = cmnStorageSyncAcquireResource(
		&gMtl4TextureStorage.textures,
		&gMtl4TextureStorage.sync,
		texture,
		&wasHandleValid
	);
	if (!wasHandleValid) {
		return nullptr;
	}

	return metadata;
}

void mtl4ReleaseTextureMetadata(void) {
	cmnStorageSyncReleaseResource(&gMtl4TextureStorage.sync);
}

GpuTextureDescriptor mtl4TextureViewDescriptor(GpuTexture texture, const GpuViewDesc* desc, GpuResult* result) {
	CmnScopedStorageSyncLockRead guard(&gMtl4TextureStorage.sync);

	GpuResult localResult;

	Mtl4Texture handle = mtl4GpuTextureToHadle(texture);

	Mtl4TextureMetadata* metadata = mtl4AcquireTextureMetadataFrom(handle);
	if (metadata == nullptr) {
		CMN_SET_RESULT(result, GPU_NO_SUCH_TEXTURE_FOUND);
		return {};
	}
	defer (mtl4ReleaseTextureMetadata());

	Mtl4TextureViews* viewsBucket;
	size_t index;
	bool didFindView = mtl4FindTextureViewIn(metadata, desc, &viewsBucket, &index);
	if (didFindView) {
		id<MTLTexture> view = viewsBucket->views[index];
		MTLResourceID resourceId = [view gpuResourceID];

		CMN_SET_RESULT(result, GPU_SUCCESS);
		return mtl4TextureResourceIdToDescriptor(resourceId);
	} else {
		MTLTextureViewDescriptor* viewDescriptor = mtl4GpuViewDescToMtl(metadata->texture, desc);

		id<MTLTexture> view = [metadata->texture newTextureViewWithDescriptor:viewDescriptor];
		if (view == nil) {
			[viewDescriptor release];
			CMN_SET_RESULT(result, GPU_GENERAL_ERROR);
			return {};
		}

		MTLResourceID resourceId = [view gpuResourceID];

		mtl4AssociateViewToTexture(metadata, view, desc, &localResult);
		if (localResult != GPU_SUCCESS) {
			[viewDescriptor release];
			[view release];

			CMN_SET_RESULT(result, localResult);
			return {};
		}

		[viewDescriptor release];
		CMN_SET_RESULT(result, GPU_SUCCESS);
		return mtl4TextureResourceIdToDescriptor(resourceId);
	}
}

GpuTextureDescriptor mtl4RWTextureViewDescriptor(GpuTexture texture, const GpuViewDesc* desc, GpuResult* result) {
	return mtl4TextureViewDescriptor(texture, desc, result);
}

bool mtl4FindTextureViewIn(Mtl4TextureMetadata* metadata, const GpuViewDesc* desc, Mtl4TextureViews** bucket, size_t* index) {
	CmnScopedReadRWMutex guard(&metadata->relatedViewsMutex);

	Mtl4TextureViews* currentBucket = metadata->relatedViews;

	size_t i;
	for (;;) {
		if (currentBucket == nullptr) {
			*bucket = nullptr;
			*index = 0;

			return false;
		}

		i = 0;
		while (
			i < 7 &&
			currentBucket->views[i] != nil &&
			memcmp(&currentBucket->textureDescriptors[i], desc, sizeof(GpuViewDesc))
		) {
			i++;
		}

		if (i < 7 && currentBucket->views[i] != nil) {
			break;
		}

		currentBucket = currentBucket->nextBucket;
	}

	*bucket = currentBucket;
	*index = i;
	return true;
}

void mtl4FreeTexture(Mtl4Texture texture) {
	Mtl4TextureMetadata* metadata = mtl4AcquireTextureMetadataFrom(texture);
	if (metadata == nullptr) {
		return;
	}
	defer (mtl4ReleaseTextureMetadata());

	cmnAtomicStore(&metadata->scheduledForDeletion, true);
	mtl4ScheduleTextureForDeletion(texture);
}

bool mtl4IsTextureScheduledForDeletion(Mtl4Texture texture) {
	Mtl4TextureMetadata* metadata = mtl4AcquireTextureMetadataFrom(texture);
	if (metadata == nullptr) {
		return false;
	}
	defer (mtl4ReleaseTextureMetadata());
	
	return cmnAtomicLoad(&metadata->scheduledForDeletion);
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

void mtl4AssociateViewToTexture(Mtl4TextureMetadata* metadata, id<MTLTexture> view, const GpuViewDesc* desc, GpuResult* result) {
	CmnResult localResult;

	CmnScopedWriteRWMutex guard(&metadata->relatedViewsMutex);

	// Find the first texture bucket free.
	Mtl4TextureViews** viewsBucketPtr = &metadata->relatedViews;
	for (;;) {
		if (*viewsBucketPtr == nullptr) {
			break;
		}

		if ((*viewsBucketPtr)->views[7 - 1] == nil) {
			break;
		}

		viewsBucketPtr = &(*viewsBucketPtr)->nextBucket;
	}

	// If there is no space, allocate a new bucket.
	Mtl4TextureViews* viewsBucket = *viewsBucketPtr;
	if (viewsBucket == nullptr) {
		viewsBucket = cmnPoolAlloc<Mtl4TextureViews>(&gMtl4TextureStorage.textureViewsPool, &localResult);
		if (localResult != CMN_SUCCESS) {
			CMN_SET_RESULT(result, GPU_OUT_OF_CPU_MEMORY);
			return;
		}

		*viewsBucketPtr = viewsBucket;
	}

	// Set the first free location in the buffer with the texture
	size_t i = 0;
	while (viewsBucket->views[i] != nil) {
		i++;
	}

	viewsBucket->views[i] = view;
	memcpy(&viewsBucket->textureDescriptors[i], desc, sizeof(GpuViewDesc));

	CMN_SET_RESULT(result, GPU_SUCCESS);
}

void mtl4FreeAssociatedTextureViews(Mtl4TextureMetadata* metadata) {
	Mtl4TextureViews* viewsBucket = metadata->relatedViews;

	while (viewsBucket != nullptr) {
		size_t i = 0;
		while (i < 7 && viewsBucket->views[i] != nil) {
			[viewsBucket->views[i] release];
			i++;
		}

		Mtl4TextureViews* nextViews = viewsBucket->nextBucket;

		cmnPoolFree(&gMtl4TextureStorage.textureViewsPool, viewsBucket);
		viewsBucket = nextViews;
	}
}

void mtl4DestroyTexture(Mtl4Texture texture) {
	bool wasHandleValid;
	Mtl4TextureMetadata* metadata = &cmnGet(&gMtl4TextureStorage.textures, texture, &wasHandleValid);
	if (!wasHandleValid) {
		return;
	}

	if (!metadata->scheduledForDeletion) {
		return;
	}

	mtl4FreeAssociatedTextureViews(metadata);
	[metadata->texture release];

	cmnRemove(&gMtl4TextureStorage.textures, texture);
}

