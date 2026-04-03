#include "context.h"

#include <stdlib.h>

#include <lib/metal4/device.h>

// 128 KB
#define MTL4_GLOBAL_MEMORY 128 * 1024

// 1 MB
#define MTL4_TEMP_MEMORY 1 * 1024 * 1024

Mtl4Context gMtl4Context;

void mtl4Init(GpuResult* result) {
	gMtl4Context.globalBackingMemory	= (uint8_t*)malloc(MTL4_GLOBAL_MEMORY);
	if (gMtl4Context.globalBackingMemory == nullptr) {
		*result = GPU_OUT_OF_CPU_MEMORY;
		goto on_error_cleanup;
	}

	gMtl4Context.tempBackingMemory		= (uint8_t*)malloc(MTL4_TEMP_MEMORY);
	if (gMtl4Context.tempBackingMemory == nullptr) {
		*result = GPU_OUT_OF_CPU_MEMORY;
		goto on_error_cleanup;
	}

	gMtl4Context.globalArena	= gpuCreateArena(gMtl4Context.globalBackingMemory, MTL4_GLOBAL_MEMORY);
	gMtl4Context.tempArena		= gpuCreateArena(gMtl4Context.tempBackingMemory, MTL4_TEMP_MEMORY);

	mtl4PrepareAvailableDevicesList(result);
	if (*result != GPU_SUCCESS) {
		goto on_error_cleanup;
	}

	*result = GPU_SUCCESS;
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

