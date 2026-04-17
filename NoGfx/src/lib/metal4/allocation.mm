#include "allocation.h"

#include <lib/common/heap_allocator.h>
#include <lib/metal4/context.h>

Mtl4AllocationStorage gMtl4AllocationStorage;

static MTLResourceOptions gMtl4ResourceOptionsFor[] = {
	/*GPU_MEMORY_DEFAULT=*/		MTLStorageModeShared | MTLResourceCPUCacheModeWriteCombined | MTLResourceHazardTrackingModeUntracked,
	/*GPU_MEMORY_GPU=*/		MTLStorageModePrivate | MTLResourceHazardTrackingModeUntracked,
	/*GPU_MEMORY_READBACK=*/	MTLStorageModeShared | MTLResourceCPUCacheModeDefaultCache | MTLResourceHazardTrackingModeUntracked
};

void mtl4InitAllocationStorage(GpuResult* result) {
	CmnResult localResult;

	CmnAllocator addressRangeMapNodesAllocator;
	CmnAllocator privateAllocationsAllocator;

	// Preallocate for more than 512k buffers
	gMtl4AllocationStorage.allocationMetadataPage = cmnCreatePage(32 * 1024 * 1024, CMN_PAGE_READABLE | CMN_PAGE_WRITABLE, &localResult);
	if (localResult != CMN_SUCCESS) {
		CMN_SET_RESULT(result, GPU_OUT_OF_CPU_MEMORY);
		goto on_error_cleanup;
	}

	// Preallocate for more than 512k buffers
	gMtl4AllocationStorage.miscArenaPage = cmnCreatePage(32 * 1024 * 1024, CMN_PAGE_READABLE | CMN_PAGE_WRITABLE, &localResult);
	if (localResult != CMN_SUCCESS) {
		CMN_SET_RESULT(result, GPU_OUT_OF_CPU_MEMORY);
		goto on_error_cleanup;
	}

	// Preallocate for more than 512k buffers
	gMtl4AllocationStorage.addressRangeMapPage = cmnCreatePage(16 * 1024 * 1024, CMN_PAGE_READABLE | CMN_PAGE_WRITABLE, &localResult);
	if (localResult != CMN_SUCCESS) {
		CMN_SET_RESULT(result, GPU_OUT_OF_CPU_MEMORY);
		goto on_error_cleanup;
	}

	gMtl4AllocationStorage.allocationMetadataPool = cmnPageToPool(
		gMtl4AllocationStorage.allocationMetadataPage,
		MTL4_ALLOCATION_METADATA_OBJECT_SIZE);
	gMtl4AllocationStorage.addressRangeMapNodesPool = cmnPageToPool(
		gMtl4AllocationStorage.addressRangeMapPage,
		sizeof(CmnBTreeNode<Mtl4AddressRange, Mtl4AllocationMetadata*>));
	gMtl4AllocationStorage.miscArena = cmnPageToArena(gMtl4AllocationStorage.addressRangeMapPage);

	addressRangeMapNodesAllocator	= cmnPoolAllocator(&gMtl4AllocationStorage.addressRangeMapNodesPool);
	privateAllocationsAllocator	= cmnArenaAllocator(&gMtl4AllocationStorage.miscArena);

	cmnCreateBTree(
		&gMtl4AllocationStorage.cpuAddressRangeMap,
		(Mtl4AllocationMetadata*)nullptr,
		addressRangeMapNodesAllocator,
		&localResult
	);
	if (localResult != CMN_SUCCESS) {
		CMN_SET_RESULT(result, GPU_OUT_OF_CPU_MEMORY);
		goto on_error_cleanup;
	}

	cmnCreateBTree(
		&gMtl4AllocationStorage.cpuAddressRangeMap,
		(Mtl4AllocationMetadata*)nullptr,
		addressRangeMapNodesAllocator,
		&localResult
	);
	if (localResult != CMN_SUCCESS) {
		CMN_SET_RESULT(result, GPU_OUT_OF_CPU_MEMORY);
		goto on_error_cleanup;
	}

	cmnCreatePointerMap(&gMtl4AllocationStorage.cpuAllocationMap, 1024, {}, cmnHeapAllocator(), &localResult);
	if (localResult != CMN_SUCCESS) {
		CMN_SET_RESULT(result, GPU_OUT_OF_CPU_MEMORY);
		goto on_error_cleanup;
	}

	cmnCreateElementPool(&gMtl4AllocationStorage.gpuAllocationPool, privateAllocationsAllocator, &localResult);
	if (localResult != CMN_SUCCESS) {
		CMN_SET_RESULT(result, GPU_OUT_OF_CPU_MEMORY);
		goto on_error_cleanup;
	}

	CMN_SET_RESULT(result, GPU_SUCCESS);
	return;

on_error_cleanup:
	mtl4FiniAllocationStorage();
}

void mtl4FiniAllocationStorage(void) {
	cmnDestroyPage(gMtl4AllocationStorage.allocationMetadataPage	);
	cmnDestroyPage(gMtl4AllocationStorage.addressRangeMapPage	);
	cmnDestroyPage(gMtl4AllocationStorage.miscArenaPage		);

	cmnDestroyPointerMap(&gMtl4AllocationStorage.cpuAllocationMap	);
	
	gMtl4AllocationStorage = {};
}

id<MTLBuffer> mtl4AllocateBuffer(size_t size, size_t align, GpuMemory memory, GpuResult* result) {
	(void)align;

	MTLResourceOptions resourceOptions = gMtl4ResourceOptionsFor[memory];

	// TODO: Overallocate to ensure alignment
	id<MTLBuffer> buffer = [gMtl4Context.device
		newBufferWithLength:size
		options: resourceOptions
	];
	if (buffer == nil) {
		CMN_SET_RESULT(result, GPU_OUT_OF_GPU_MEMORY);
	}

	CMN_SET_RESULT(result, GPU_SUCCESS);
	return buffer;
}

void* mtl4MallocDirectMemory(size_t size, size_t align, GpuMemory memory, GpuResult* result) {
	CmnResult localResult;
	GpuResult localGpuResult;

	Mtl4AllocationMetadata* metadata = nullptr;
	Mtl4AddressRange cpuRange	= {};
	void* cpuAddress		= nullptr;
	size_t gpuAllocationIndex	= 0;

	id<MTLBuffer> buffer = mtl4AllocateBuffer(size, align, memory, &localGpuResult);
	if (localGpuResult != GPU_SUCCESS) {
		CMN_SET_RESULT(result, localGpuResult);
		return nullptr;
	}

	metadata = cmnPoolAlloc<Mtl4AllocationMetadata>(
		&gMtl4AllocationStorage.allocationMetadataPool,
		&localResult
	);
	if (localResult != CMN_SUCCESS) {
		CMN_SET_RESULT(result, GPU_OUT_OF_CPU_MEMORY);
		goto on_error_cleanup;
	}

	cpuAddress = [buffer contents];

	assert(mtl4IsCpuAddress(cpuAddress));


	cpuRange.start	= (uintptr_t)cpuAddress;
	cpuRange.length	= size;

	{
		CmnScopedMutex guard(&gMtl4AllocationStorage.mutex);

		gpuAllocationIndex = cmnInsert(&gMtl4AllocationStorage.gpuAllocationPool, {}, &localResult);
		if (localResult != CMN_SUCCESS) {
			CMN_SET_RESULT(result, GPU_OUT_OF_CPU_MEMORY);
			goto on_error_cleanup;
		}

		metadata->buffer		= buffer;
		metadata->size			= size;
		metadata->align			= align;
		metadata->memory		= memory;
		metadata->cpuAddress		= cpuAddress;
		metadata->gpuAddress.guard	= 1;
		metadata->gpuAddress.offset	= 0;
		metadata->gpuAddress.allocationIdentifier = gpuAllocationIndex;

		gMtl4AllocationStorage.gpuAllocationPool[gpuAllocationIndex] = metadata;

		cmnInsert(&gMtl4AllocationStorage.cpuAddressRangeMap, cpuRange, metadata, &localResult);
		if (localResult != CMN_SUCCESS) {
			CMN_SET_RESULT(result, GPU_OUT_OF_CPU_MEMORY);
			goto on_error_cleanup;
		}

		cmnInsert(&gMtl4AllocationStorage.cpuAllocationMap, (uintptr_t)cpuAddress, metadata, &localResult);
		if (localResult != CMN_SUCCESS) {
			CMN_SET_RESULT(result, GPU_OUT_OF_CPU_MEMORY);
			goto on_error_cleanup;
		}
	}

	CMN_SET_RESULT(result, GPU_SUCCESS);
	return cpuAddress;

on_error_cleanup:
	if (gpuAllocationIndex != 0) {
		cmnRemove(&gMtl4AllocationStorage.gpuAllocationPool, gpuAllocationIndex);
	}
	cmnRemove(&gMtl4AllocationStorage.cpuAddressRangeMap, cpuRange);
	cmnRemove(&gMtl4AllocationStorage.cpuAllocationMap, (uintptr_t)cpuAddress);

	cmnPoolFree(&gMtl4AllocationStorage.allocationMetadataPool, metadata);
	if (buffer != nil) {
		[buffer release];
	}

	return nullptr;
}

void* mtl4MallocVirtualMemory(size_t size, size_t align, GpuResult* result) {
	(void)align;
	CmnResult localResult;

	Mtl4AllocationMetadata* metadata = cmnPoolAlloc<Mtl4AllocationMetadata>(
		&gMtl4AllocationStorage.allocationMetadataPool,
		&localResult
	);
	if (localResult != CMN_SUCCESS) {
		CMN_SET_RESULT(result, GPU_OUT_OF_CPU_MEMORY);
		return nullptr;
	}

	Mtl4GpuAddress address;

	{
		CmnScopedMutex guard(&gMtl4AllocationStorage.mutex);

		size_t metadataIndex = cmnInsert(&gMtl4AllocationStorage.gpuAllocationPool, {}, &localResult);
		if (localResult != CMN_SUCCESS) {
			CMN_SET_RESULT(result, GPU_OUT_OF_CPU_MEMORY);
			goto on_error_cleanup;
		}

		metadata->buffer	= nil;
		metadata->size		= size;
		metadata->align		= align;
		metadata->memory	= GPU_MEMORY_GPU;
		gMtl4AllocationStorage.gpuAllocationPool[metadataIndex] = metadata;

		// NOTE: Avoid using index 0, as it indicates a _gpu virtual address_ of a direct allocation
		address.allocationIdentifier	= metadataIndex;
		address.guard			= true;
		address.offset			= 0;

		metadata->gpuAddress	= address;
	}

	CMN_SET_RESULT(result, GPU_SUCCESS);
	return mtl4GpuAddressToPtr(address);

on_error_cleanup:
	cmnPoolFree(&gMtl4AllocationStorage.allocationMetadataPool, metadata);
	return nullptr;
}

void* mtl4Malloc(size_t size, size_t align, GpuMemory memory, GpuResult* result) {
	switch (memory) {
		case GPU_MEMORY_DEFAULT:
		case GPU_MEMORY_READBACK: {
			return mtl4MallocDirectMemory(size, align, memory, result);
		}
		case GPU_MEMORY_GPU: {
			return mtl4MallocVirtualMemory(size, align, result);
		}
	}

	return nullptr;
}

void  mtl4Free(void* ptr) {
	if (ptr == nullptr) {
		return;
	}


	Mtl4AllocationTextures* texturesToFree;
	{
		CmnScopedMutex guard(&gMtl4AllocationStorage.mutex);

		Mtl4AllocationMetadata* metadata = mtl4GetAllocationMetadataOf(ptr, false);
		if (metadata == nullptr) {
			return;
		}

		texturesToFree = metadata->relatedTextures;

		Mtl4AddressRange range;
		range.start = (uintptr_t)ptr;
		range.length = 0;

		cmnRemove(&gMtl4AllocationStorage.cpuAddressRangeMap, range);
		cmnRemove(&gMtl4AllocationStorage.cpuAllocationMap, (uintptr_t)ptr);
		cmnRemove(&gMtl4AllocationStorage.gpuAllocationPool, metadata->gpuAddress.allocationIdentifier);

		if (metadata->buffer != nil) {
			[metadata->buffer release];
		}

		cmnPoolFree(&gMtl4AllocationStorage.allocationMetadataPool, metadata);
	}

	// NOTE: Needs to be done separately, since this calls the texture storage, thus locking it.
	//	If this gets done when also the buffer storage gets locked, a deadlock could occurr.
	{
		mtl4FreeAssociatedTextures(texturesToFree);
	}
}

uintptr_t mtl4GpuAddressToActual(void* gpuPtr) {
	CmnScopedMutex guard(&gMtl4AllocationStorage.mutex);

	Mtl4GpuAddress address = mtl4PtrToGpuAddress(gpuPtr);

	Mtl4AllocationMetadata* metadata = mtl4GetAllocationMetadataOfGpuPtr(gpuPtr);
	if (metadata == nullptr) {
		return 0;
	}

	if (metadata->buffer == nil) {
		return 0;
	}

	return metadata->buffer.gpuAddress + address.offset;
}

Mtl4AllocationMetadata* mtl4GetAllocationMetadataOf(void* ptr, bool attemptRangeBasedLookup) {
	if (mtl4IsCpuAddress(ptr)) {
		return mtl4GetAllocationMetadataOfCpuPtr(ptr, attemptRangeBasedLookup);
	} else {
		return mtl4GetAllocationMetadataOfGpuPtr(ptr);
	}

	return nullptr;
}

Mtl4AllocationMetadata* mtl4GetAllocationMetadataOfCpuPtr(void* ptr, bool attemptRangeBasedLookup) {
	assert(mtl4IsCpuAddress(ptr));

	bool didFindElement;
	Mtl4AllocationMetadata* metadata;

	// NOTE: Attempt fast lookup. Will find the address if no offset has beed applied to the pointer.
	metadata = cmnGet(&gMtl4AllocationStorage.cpuAllocationMap, (uintptr_t)ptr, &didFindElement);
	if (didFindElement) {
		return metadata;
	}

	if (!attemptRangeBasedLookup) {
		return nullptr;
	}
	
	// NOTE: Slow lookup
	Mtl4AddressRange range;
	range.start = (uintptr_t)ptr;
	range.length = 0;

	metadata = cmnGet(&gMtl4AllocationStorage.cpuAddressRangeMap, range, &didFindElement);
	if (didFindElement) {
		return metadata;
	}

	return nullptr;
}

Mtl4AllocationMetadata* mtl4GetAllocationMetadataOfGpuPtr(Mtl4GpuAddress address) {
	if (address.guard == 0) {
		return nullptr;
	}

	return gMtl4AllocationStorage.gpuAllocationPool[address.allocationIdentifier];
}


void* mtl4HostToDevicePointer(void* ptr, GpuResult* result) {
	CmnScopedMutex guard(&gMtl4AllocationStorage.mutex);

	if (mtl4IsGpuAddress(ptr)) {
		CMN_SET_RESULT(result, GPU_ALLOCATION_MEMORY_IS_GPU);
		return nullptr;
	}

	Mtl4AllocationMetadata* metadata = mtl4GetAllocationMetadataOf(ptr, true);
	if (metadata == nullptr) {
		CMN_SET_RESULT(result, GPU_NO_SUCH_ALLOCATION_FOUND);
		return nullptr;
	}

	CMN_SET_RESULT(result, GPU_SUCCESS);

	uintptr_t offsetFromBase = (uintptr_t)ptr - (uintptr_t)metadata->cpuAddress;

	Mtl4GpuAddress address = metadata->gpuAddress;
	address.offset = offsetFromBase;
	return mtl4GpuAddressToPtr(address);
}

void mtl4AssociateTextureToAllocation(Mtl4AllocationMetadata* metadata, Mtl4Texture texture, GpuResult* result) {
	CmnResult localResult;

	// Find the first texture bucket free.
	Mtl4AllocationTextures** texturesPtr = &metadata->relatedTextures;
	for (;;) {
		if (*texturesPtr == nullptr) {
			break;
		}

		if (cmnIsZero((*texturesPtr)->textures[MTL4_TEXTURES_PER_ALLOCATION_TEXTURE_BUCKET - 1])) {
			break;
		}

		texturesPtr = &(*texturesPtr)->nextBucket;
	}

	// If there is no space, allocate a new bucket.
	Mtl4AllocationTextures* textures = *texturesPtr;
	if (textures == nullptr) {
		textures = cmnPoolAlloc<Mtl4AllocationTextures>(&gMtl4AllocationStorage.allocationMetadataPool, &localResult);
		if (localResult != CMN_SUCCESS) {
			CMN_SET_RESULT(result, GPU_OUT_OF_CPU_MEMORY);
			return;
		}

		*texturesPtr = textures;
	}

	// Set the first free location in the buffer with the texture
	size_t i = 0;
	while (cmnIsZero(textures->textures[i])) {
		i++;
	}

	textures->textures[i] = texture;

	CMN_SET_RESULT(result, GPU_SUCCESS);
}

void mtl4FreeAssociatedTextures(Mtl4AllocationTextures* textureBucket) {
	Mtl4AllocationTextures* textures = textureBucket;

	while (textures != nullptr) {
		size_t i = 0;
		while (cmnIsZero(textures->textures[i])) {
			mtl4DestroyTexture(textures->textures[i]);
			i++;
		}


		Mtl4AllocationTextures* nextTextures = textures = textures->nextBucket;

		cmnPoolFree(&gMtl4AllocationStorage.allocationMetadataPool, textures);
		textures = nextTextures;
	}
}

void mtl4EnsureBackingBufferIsAllocated(Mtl4GpuAddress address, GpuResult* result) {
	CmnScopedMutex guard(&gMtl4AllocationStorage.mutex);

	Mtl4AllocationMetadata* metadata = mtl4GetAllocationMetadataOfGpuPtr(address);

	if (metadata->buffer == nil) {
		CMN_SET_RESULT(result, GPU_SUCCESS);
		return;
	}

	metadata->buffer = mtl4AllocateBuffer(metadata->size, metadata->align, metadata->memory, result);
}
