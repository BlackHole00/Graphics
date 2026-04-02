#ifndef GPU_METAL4CONTEXT_H
#define GPU_METAL4CONTEXT_H

#include <gpu/gpu.h>
#include <lib/common/common.h>
#include <lib/common/arena.h>

typedef struct {
	GpuArena globalArena;
	uint8_t* globalBackingMemory;

	GpuArena tempArena;
	uint8_t* tempBackingMemory;
} Mtl4Context;
extern Mtl4Context gMtl4Context;

void mtl4Init(GpuResult* result);

#endif // GPU_METAL4CONTEXT_H

