#include "context.h"

#include <stdlib.h>

#include <lib/common/type_traits.h>
#include <lib/metal4/device.h>
#include <lib/metal4/allocation.h>

// 128 KB
#define MTL4_GLOBAL_MEMORY 128 * 1024

// 1 MB
#define MTL4_TEMP_MEMORY 1 * 1024 * 1024

Mtl4Context gMtl4Context;

void mtl4Init(GpuResult* result) {
	GpuResult localResult;

	gMtl4Context.globalBackingMemory	= (uint8_t*)malloc(MTL4_GLOBAL_MEMORY);
	if (gMtl4Context.globalBackingMemory == nullptr) {
		CMN_SET_RESULT(result, GPU_OUT_OF_CPU_MEMORY);
		goto on_error_cleanup;
	}

	gMtl4Context.tempBackingMemory		= (uint8_t*)malloc(MTL4_TEMP_MEMORY);
	if (gMtl4Context.tempBackingMemory == nullptr) {
		CMN_SET_RESULT(result, GPU_OUT_OF_CPU_MEMORY);
		goto on_error_cleanup;
	}

	gMtl4Context.globalArena	= cmnCreateArena(gMtl4Context.globalBackingMemory, MTL4_GLOBAL_MEMORY, true);
	gMtl4Context.tempArena		= cmnCreateArena(gMtl4Context.tempBackingMemory, MTL4_TEMP_MEMORY, true);

	mtl4PrepareAvailableDevicesList(&localResult);
	if (localResult != GPU_SUCCESS) {
		CMN_SET_RESULT(result, localResult);
		goto on_error_cleanup;
	}

	mtl4PrepareAllocationStorage(&localResult);
	if (localResult != GPU_SUCCESS) {
		CMN_SET_RESULT(result, localResult);
		goto on_error_cleanup;
	}

	CMN_SET_RESULT(result, GPU_SUCCESS);
	return;

on_error_cleanup:
	free(gMtl4Context.globalBackingMemory);
	free(gMtl4Context.tempBackingMemory);

	return;
}

void mtl4Deinit(void) {
	if (gMtl4Context.device != nullptr) {
		[gMtl4Context.device release];
	}

	free(gMtl4Context.globalBackingMemory);
	free(gMtl4Context.tempBackingMemory);
}

