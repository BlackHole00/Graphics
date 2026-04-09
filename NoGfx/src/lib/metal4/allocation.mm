#include "allocation.h"

#include <lib/metal4/context.h>

Mtl4AllocationStorage gMtl4AllocationStorage;

void mtl4PrepareAllocationStorage(GpuResult* result) {
	CmnResult localResult;

	gMtl4AllocationStorage.allocationMetadataPage = cmnCreatePage(1 * 1024 * 1024, CMN_PAGE_READABLE | CMN_PAGE_WRITABLE, &localResult);
	if (localResult != CMN_SUCCESS) {
		CMN_SET_RESULT(result, GPU_OUT_OF_CPU_MEMORY);
		goto on_error_cleanup;
	}

	gMtl4AllocationStorage.addressRangeMapPage = cmnCreatePage(1 * 1024 * 1024, CMN_PAGE_READABLE | CMN_PAGE_WRITABLE, &localResult);
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

	cmnCreateBTree(
		&gMtl4AllocationStorage.addressRangeMap,
		(Mtl4AllocationMetadata*)nullptr,
		&gMtl4AllocationStorage.addressRangeMapNodesPool,
		&localResult
	);
	if (localResult != CMN_SUCCESS) {
		CMN_SET_RESULT(result, GPU_OUT_OF_CPU_MEMORY);
		goto on_error_cleanup;
	}

	CMN_SET_RESULT(result, GPU_SUCCESS);
	return;

on_error_cleanup:
	cmnDestroyPage(gMtl4AllocationStorage.allocationMetadataPage	);
	cmnDestroyPage(gMtl4AllocationStorage.addressRangeMapPage	);
}


void* mtl4MallocDefaultMemory(size_t size, size_t align, GpuResult* result) {
	(void)align;
	CmnResult localResult;

	Mtl4AllocationMetadata* metadata = nullptr;

	// TODO: Overallocate to handle alignment
	id<MTLBuffer> buffer = [gMtl4Context.device
		newBufferWithLength: size
		options: MTLStorageModeShared | MTLResourceCPUCacheModeWriteCombined | MTLResourceHazardTrackingModeUntracked
	];
	if (buffer == nil) {
		CMN_SET_RESULT(result, GPU_OUT_OF_GPU_MEMORY);
		goto on_error_cleanup;
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
	metadata->memory	= GPU_MEMORY_GPU;
	metadata->cpuAddress	= [buffer contents];
	metadata->gpuAddress	= (void*)[buffer gpuAddress];

	Mtl4AddressRange range;
	range.start	= (uintptr_t)metadata->cpuAddress;
	range.length	= size;
	cmnInsert(&gMtl4AllocationStorage.addressRangeMap, range, metadata, &localResult);
	if (localResult != CMN_SUCCESS) {
		CMN_SET_RESULT(result, GPU_OUT_OF_CPU_MEMORY);
		goto on_error_cleanup;
	}

	CMN_SET_RESULT(result, GPU_SUCCESS);
	return metadata->cpuAddress;

on_error_cleanup:
	if (buffer != nil) {
		[buffer release];
	}
	cmnPoolFree(&gMtl4AllocationStorage.allocationMetadataPool, metadata);

	return nullptr;
}

void* mtl4Malloc(size_t size, size_t align, GpuMemory memory, GpuResult* result) {
	switch (memory) {
		case GPU_MEMORY_DEFAULT: {
			return mtl4MallocDefaultMemory(size, align, result);
		}
		case GPU_MEMORY_GPU: {
			break;
		}
		case GPU_MEMORY_READBACK: {
			break;
		}
	}

	return nullptr;
}

void  mtl4Free(void* ptr) {}

void* mtl4HostToDevicePointer(void* ptr, GpuResult* result) {
	Mtl4AddressRange range;
	range.start = (uintptr_t)ptr;
	range.length = 0;

	bool didFindElement;
	Mtl4AllocationMetadata* metadata = cmnGet(&gMtl4AllocationStorage.addressRangeMap, range, &didFindElement);
	if (!didFindElement) {
		CMN_SET_RESULT(result, GPU_NO_SUCH_ALLOCATION_FOUND);
		return nullptr;
	}

	uintptr_t offsetFromBase = (uintptr_t)ptr - (uintptr_t)metadata->cpuAddress;
	uintptr_t gpuAddress = (uintptr_t)metadata->gpuAddress + offsetFromBase;
	return (void*)gpuAddress;
}

