#ifndef GPU_METAL4CONTEXT_H
#define GPU_METAL4CONTEXT_H

#include <gpu/gpu.h>
#include <lib/common/common.h>
#include <lib/common/arena.h>

#include <Metal/Metal.h>

typedef struct {
	GpuArena	globalArena;
	uint8_t*	globalBackingMemory;

	GpuArena	tempArena;
	uint8_t*	tempBackingMemory;

	id<MTLDevice>	device;
	GpuDeviceId	selectedDeviceId;
} Mtl4Context;
extern Mtl4Context gMtl4Context;

void mtl4Init(GpuResult* result);
void mtl4Deinit(void);

#endif // GPU_METAL4CONTEXT_H

