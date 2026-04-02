#include "device.h"

#include <lib/metal4/context.h>
#include <Metal/Metal.h>

typedef struct {
	GpuDeviceInfo*	devices;
	size_t		count;
} Mtl4AvailableDevicesList;
Mtl4AvailableDevicesList gMtl4AvailableDevicesList;

bool mtl4CheckDeviceSuitability(id<MTLDevice> device) {
	return device.hasUnifiedMemory &&
		[device supportsFamily:MTLGPUFamilyMetal4];
}

void mtl4PrepareAvailableDevicesList(GpuResult* result) {
	GpuArena* arena = &gMtl4Context.globalArena;
	GpuArenaState onErrorRecoveryState = gpuBeginArenaTemp(arena);

	NSArray<id<MTLDevice>>* mtlDevices = MTLCopyAllDevices();
	GpuDeviceInfo* devicesInfo = gpuArenaAlloc<GpuDeviceInfo>(arena, [mtlDevices count]);
	if (devicesInfo == nullptr) {
		*result = GPU_OUT_OF_CPU_MEMORY;
		goto on_error_cleanup;
	}

	GpuDeviceId deviceId; deviceId = 0;

	for (size_t i = 0; i < [mtlDevices count]; i++) {
		id<MTLDevice> mtlDevice = mtlDevices[i];
		GpuDeviceInfo* deviceInfo = &devicesInfo[i];

		if (!mtl4CheckDeviceSuitability(mtlDevice)) {
			continue;
		}

		size_t deviceNameLength = [[mtlDevice name] maximumLengthOfBytesUsingEncoding:NSUTF8StringEncoding];
		deviceInfo->name = gpuArenaAlloc<char>(arena, deviceNameLength);
		if (deviceInfo->name == nullptr) {
			*result = GPU_OUT_OF_CPU_MEMORY;
			goto on_error_cleanup;
		}
		[[mtlDevice name] getCString:(char*)deviceInfo->name
			maxLength:deviceNameLength
			encoding:NSUTF8StringEncoding];

		// TODO: Actually check for other types of vendor. This should be fine, since only Apple hardware is
		//	supported.
		deviceInfo->vendor = "Apple";

		deviceInfo->identifier = deviceId;
		deviceInfo->type = [mtlDevice isLowPower] ? GPU_INTEGRATED : GPU_DEDICATED;

		deviceId++;
	}

	gMtl4AvailableDevicesList.devices = devicesInfo;
	gMtl4AvailableDevicesList.count = [mtlDevices count];

	[mtlDevices release];

	*result = GPU_SUCCESS;
	return;

on_error_cleanup:
	gMtl4AvailableDevicesList.devices = nullptr;
	gMtl4AvailableDevicesList.count = 0;

	gpuEndArenaTemp(onErrorRecoveryState);
	[mtlDevices release];
}

void mtl4EnumerateDevices(GpuDeviceInfo** devices, size_t* devices_count, GpuResult* result) {
	*devices = gMtl4AvailableDevicesList.devices;
	*devices_count = gMtl4AvailableDevicesList.count;

	*result = GPU_SUCCESS;
	return;
}

