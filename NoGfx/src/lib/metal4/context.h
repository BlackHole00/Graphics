#ifndef GPU_METAL4CONTEXT_H
#define GPU_METAL4CONTEXT_H

#include <gpu/gpu.h>
#include <lib/common/common.h>
#include <lib/common/arena.h>
#include <lib/metal4/device.h>

#include <Metal/Metal.h>

typedef struct {
	GpuDeviceInfo*	info;
	id<MTLDevice>*	devices;

	size_t		count;
} Mtl4AvailableDevicesList;

typedef struct {
	CmnArena	globalArena;
	uint8_t*	globalBackingMemory;

	CmnArena	tempArena;
	uint8_t*	tempBackingMemory;

	bool		shouldTrace;
	bool		isCurrentlyTracing;

	Mtl4AvailableDevicesList availableDevices;
	id<MTLDevice>	device;
	GpuDeviceId	selectedDeviceId;
} Mtl4Context;
extern Mtl4Context gMtl4Context;

void mtl4Init(const GpuInitDesc* desc, GpuResult* result);
void mtl4Deinit(void);

#endif // GPU_METAL4CONTEXT_H

