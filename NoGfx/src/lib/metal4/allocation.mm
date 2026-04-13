#include "allocation.h"

#include <lib/common/heap_allocator.h>
#include <lib/metal4/context.h>

Mtl4AllocationStorage gMtl4AllocationStorage;

void mtl4InitAllocationStorage(GpuResult* result) {
	CmnResult localResult;
	CmnAllocator addressRangeMapNodesAllocator;

	// Preallocate for more than 512k buffers
	gMtl4AllocationStorage.allocationMetadataPage = cmnCreatePage(24 * 1024 * 1024, CMN_PAGE_READABLE | CMN_PAGE_WRITABLE, &localResult);
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
		sizeof(Mtl4AllocationMetadata));
	gMtl4AllocationStorage.addressRangeMapNodesPool = cmnPageToPool(
		gMtl4AllocationStorage.addressRangeMapPage,
		sizeof(CmnBTreeNode<Mtl4AddressRange, Mtl4AllocationMetadata*>));

	addressRangeMapNodesAllocator = cmnPoolAllocator(&gMtl4AllocationStorage.addressRangeMapNodesPool);

	cmnCreateBTree(
		&gMtl4AllocationStorage.addressRangeMap,
		(Mtl4AllocationMetadata*)nullptr,
		addressRangeMapNodesAllocator,
		&localResult
	);
	if (localResult != CMN_SUCCESS) {
		CMN_SET_RESULT(result, GPU_OUT_OF_CPU_MEMORY);
		goto on_error_cleanup;
	}

	cmnCreatePointerMap(&gMtl4AllocationStorage.allocationMap, 1024, {}, cmnHeapAllocator(), &localResult);
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
	cmnDestroyPage(gMtl4AllocationStorage.allocationMetadataPage);
	cmnDestroyPage(gMtl4AllocationStorage.addressRangeMapPage);
	cmnDestroyPointerMap(&gMtl4AllocationStorage.allocationMap);
	
	gMtl4AllocationStorage = {};
}

Mtl4AllocationMetadata* mtl4GetAllocationMetadataOf(void* ptr, bool attemptRangeBasedLookup) {
	bool didFindElement;
	Mtl4AllocationMetadata* metadata;

	// NOTE: Attempt fast lookup. Will find the address if no offset has beed applied to the pointer.
	metadata = cmnGet(&gMtl4AllocationStorage.allocationMap, (uintptr_t)ptr, &didFindElement);
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

	metadata = cmnGet(&gMtl4AllocationStorage.addressRangeMap, range, &didFindElement);
	if (didFindElement) {
		return metadata;
	}

	return nullptr;
}

void* mtl4MallocCpuAccessibleMemory(size_t size, size_t align, bool optimizeForReadback, GpuResult* result) {
	(void)align;
	CmnResult localResult;

	Mtl4AllocationMetadata* metadata = nullptr;

	MTLResourceOptions resourceOptions;
	if (optimizeForReadback) {
		resourceOptions = MTLStorageModeShared | MTLResourceCPUCacheModeDefaultCache | MTLResourceHazardTrackingModeUntracked;
	} else {
		resourceOptions = MTLStorageModeShared | MTLResourceCPUCacheModeWriteCombined | MTLResourceHazardTrackingModeUntracked;
	}

	// TODO: Overallocate to handle alignment
	id<MTLBuffer> buffer = [gMtl4Context.device
		newBufferWithLength: size
		options: resourceOptions
	];
	if (buffer == nil) {
		CMN_SET_RESULT(result, GPU_OUT_OF_GPU_MEMORY);
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

	metadata->buffer	= buffer;
	metadata->size		= size;
	metadata->memory	= optimizeForReadback ? GPU_MEMORY_READBACK : GPU_MEMORY_DEFAULT;
	metadata->cpuAddress	= [buffer contents];
	metadata->gpuAddress	= (void*)[buffer gpuAddress];

	Mtl4AddressRange cpuRange;
	cpuRange.start	= (uintptr_t)metadata->cpuAddress;
	cpuRange.length	= size;
	Mtl4AddressRange gpuRange;
	gpuRange.start	= (uintptr_t)metadata->gpuAddress;
	gpuRange.length	= size;

	{
		CmnScopedMutex guard(&gMtl4AllocationStorage.allocationMutex);

		cmnInsert(&gMtl4AllocationStorage.addressRangeMap, cpuRange, metadata, &localResult);
		if (localResult != CMN_SUCCESS) {
			CMN_SET_RESULT(result, GPU_OUT_OF_CPU_MEMORY);
			cmnRemove(&gMtl4AllocationStorage.allocationMap, (uintptr_t)metadata->cpuAddress);
			goto on_error_cleanup;
		}

		cmnInsert(&gMtl4AllocationStorage.addressRangeMap, gpuRange, metadata, &localResult);
		if (localResult != CMN_SUCCESS) {
			CMN_SET_RESULT(result, GPU_OUT_OF_CPU_MEMORY);
			cmnRemove(&gMtl4AllocationStorage.allocationMap, (uintptr_t)metadata->cpuAddress);
			goto on_error_cleanup;
		}

		cmnInsert(&gMtl4AllocationStorage.allocationMap, (uintptr_t)metadata->cpuAddress, metadata, &localResult);
		if (localResult != CMN_SUCCESS) {
			CMN_SET_RESULT(result, GPU_OUT_OF_CPU_MEMORY);
			goto on_error_cleanup;
		}
	}

	CMN_SET_RESULT(result, GPU_SUCCESS);
	return metadata->cpuAddress;

on_error_cleanup:
	if (buffer != nil) {
		[buffer release];
	}
	cmnRemove(&gMtl4AllocationStorage.addressRangeMap, cpuRange);
	cmnRemove(&gMtl4AllocationStorage.addressRangeMap, gpuRange);
	cmnPoolFree(&gMtl4AllocationStorage.allocationMetadataPool, metadata);

	return nullptr;
}

void* mtl4MallocGpuOnlyMemory(size_t size, size_t align, GpuResult* result) {
	(void)align;
	CmnResult localResult;

	id<MTLBuffer> buffer = [gMtl4Context.device
	 	newBufferWithLength:size
	 	options:MTLResourceStorageModePrivate];
	if (buffer == nil) {
		CMN_SET_RESULT(result, GPU_OUT_OF_GPU_MEMORY);
		return nullptr;
	}

	Mtl4AllocationMetadata* metadata = cmnPoolAlloc<Mtl4AllocationMetadata>(
		&gMtl4AllocationStorage.allocationMetadataPool,
		&localResult
	);
	if (localResult != CMN_SUCCESS) {
		CMN_SET_RESULT(result, GPU_OUT_OF_CPU_MEMORY);
		goto on_error_cleanup;
	}

	Mtl4AddressRange gpuRange;
	gpuRange.start	= (uintptr_t)metadata->gpuAddress;
	gpuRange.length	= size;

	{
		CmnScopedMutex guard(&gMtl4AllocationStorage.allocationMutex);

		metadata->buffer = buffer;
		metadata->memory = GPU_MEMORY_GPU;
		metadata->gpuAddress = (void*)[buffer gpuAddress];
		metadata->size = size;

		cmnInsert(&gMtl4AllocationStorage.addressRangeMap, gpuRange, metadata, &localResult);
		if (localResult != CMN_SUCCESS) {
			CMN_SET_RESULT(result, GPU_OUT_OF_CPU_MEMORY);
			cmnRemove(&gMtl4AllocationStorage.allocationMap, (uintptr_t)metadata->cpuAddress);
			goto on_error_cleanup;
		}

		cmnInsert(&gMtl4AllocationStorage.allocationMap, (uintptr_t)metadata->gpuAddress, metadata, &localResult);
		if (localResult != CMN_SUCCESS) {
			CMN_SET_RESULT(result, GPU_OUT_OF_CPU_MEMORY);
			goto on_error_cleanup;
		}
	}

	return metadata->gpuAddress;

on_error_cleanup:
	if (buffer != nil) {
		[buffer release];
	}
	cmnRemove(&gMtl4AllocationStorage.addressRangeMap, gpuRange);
	cmnPoolFree(&gMtl4AllocationStorage.allocationMetadataPool, metadata);

	return nullptr;
}

void* mtl4Malloc(size_t size, size_t align, GpuMemory memory, GpuResult* result) {
	switch (memory) {
		case GPU_MEMORY_DEFAULT: {
			return mtl4MallocCpuAccessibleMemory(size, align, false, result);
		}
		case GPU_MEMORY_READBACK: {
			return mtl4MallocCpuAccessibleMemory(size, align, true, result);
		}
		case GPU_MEMORY_GPU: {
			return mtl4MallocGpuOnlyMemory(size, align, result);
		}
	}

	return nullptr;
}

void  mtl4Free(void* ptr) {
	if (ptr == nullptr) {
		return;
	}

	CmnScopedMutex guard(&gMtl4AllocationStorage.allocationMutex);

	Mtl4AllocationMetadata* metadata = mtl4GetAllocationMetadataOf(ptr, false);
	if (metadata == nullptr) {
		return;
	}

	Mtl4AddressRange range;
	range.start = (uintptr_t)ptr;
	range.length = 0;

	cmnRemove(&gMtl4AllocationStorage.addressRangeMap, range);
	cmnRemove(&gMtl4AllocationStorage.allocationMap, (uintptr_t)ptr);

	[metadata->buffer release];
}

void* mtl4HostToDevicePointer(void* ptr, GpuResult* result) {
	Mtl4AllocationMetadata* metadata;

	{
		CmnScopedMutex guard(&gMtl4AllocationStorage.allocationMutex);
		metadata = mtl4GetAllocationMetadataOf(ptr, true);
	}

	if (metadata == nullptr) {
		CMN_SET_RESULT(result, GPU_NO_SUCH_ALLOCATION_FOUND);
		return nullptr;
	}

	if (metadata->memory == GPU_MEMORY_GPU) {
		CMN_SET_RESULT(result, GPU_ALLOCATION_MEMORY_IS_GPU);
		return nullptr;
	}

	CMN_SET_RESULT(result, GPU_SUCCESS);

	uintptr_t offsetFromBase = (uintptr_t)ptr - (uintptr_t)metadata->cpuAddress;
	uintptr_t gpuAddress = (uintptr_t)metadata->gpuAddress + offsetFromBase;
	return (void*)gpuAddress;
}

