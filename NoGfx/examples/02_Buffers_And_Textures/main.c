#include <stdio.h>
#include <gpu/gpu.h>

#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"

GpuBackend selectBackend(void) {
	#ifdef __APPLE__
		return GPU_METAL_4;
	#else
		return GPU_VULKAN;
	#endif
}

typedef struct GpuAllocation {
	uint8_t*	cpu;
	uint8_t*	gpu;
} GpuAllocation;

typedef struct GpuBumpAllocator {
	uint8_t*	cpu;
	uint8_t*	gpu;
	uint32_t	size;
	uint32_t	offset;
} GpuBumpAllocator;

void createGpuBumpAllocator(GpuBumpAllocator* allocator, size_t size) {
	allocator->cpu = (uint8_t*)gpuMalloc(size, 16, GPU_MEMORY_DEFAULT, NULL);
	allocator->gpu = (uint8_t*)gpuHostToDevicePointer(allocator->cpu, NULL);
	allocator->offset = 0;
	allocator->size = size;
}

GpuAllocation bumpAlloc(GpuBumpAllocator* allocator, size_t bytes) {
	if (allocator->offset + bytes >= allocator->size) {
		allocator->offset = 0;
	}

	GpuAllocation alloc;
	alloc.cpu = allocator->cpu + allocator->offset;
	alloc.gpu = allocator->gpu + allocator->offset;

	allocator->offset += bytes;

	return alloc;
}


int main(void) {
	GpuInitDesc desc;
	desc.backend		= selectBackend();
	desc.validationEnabled	= true;
	desc.extraLayers	= NULL;
	desc.extraLayerCount	= 0;

	GpuResult result = GPU_SUCCESS;

	gpuInit(&desc, &result);
	if (result != GPU_SUCCESS) {
		printf("Failed to initalize NoGfx. Got error %d.\n", result);
		return -1;
	}

	GpuDeviceInfo* devices;
	size_t devices_count;
	gpuEnumerateDevices(&devices, &devices_count, &result);
	if (result != GPU_SUCCESS) {
		printf("Failed to get the available devices. Got error %d.\n", result);
		return -1;
	}

	printf("Available devices:\n");
	for (size_t i = 0; i < devices_count; i++) {
		GpuDeviceInfo* info = &devices[i];

		printf(
			"\t%u - %s (%s - %s)\n",
			(unsigned int)info->identifier,
			info->name,
			info->vendor,
			info->type == GPU_INTEGRATED ? "Integrated" : "Dedicated"
		);
	}

	if (devices_count <= 0) {
		printf("No available devices found. Aborting.\n");
		return -1;
	}

	gpuSelectDevice(devices[0].identifier, &result);
	if (result != GPU_SUCCESS) {
		printf("Could not select a the specified device. Aborting.\n");
		return -1;
	}
	printf("Using device `%s`.\n", devices[0].name);

	GpuBumpAllocator bumpAllocator;
	createGpuBumpAllocator(&bumpAllocator, 1 * 1024 * 1024);

	FILE* f = fopen("image.png", "r");
	int x, y, channels;
	uint8_t* data = stbi_load_from_file(f, &x, &y, &channels, 4);
	assert(channels == 4);

	fclose(f);

	GpuAllocation gpuTempData = bumpAlloc(&bumpAllocator, x * y * channels);
	memcpy(gpuTempData.cpu, data, x * y * channels);

	GpuTextureDesc textureDescriptor = {};
	textureDescriptor.type = GPU_TEXTURE_3D;
	textureDescriptor.format = GPU_FORMAT_RGBA8_UNORM;
	textureDescriptor.usage = GPU_USAGE_SAMPLED;
	textureDescriptor.dimensions[0] = x;
	textureDescriptor.dimensions[1] = y;
	textureDescriptor.dimensions[2] = 1;
	textureDescriptor.layerCount = 1;
	textureDescriptor.mipCount = 1;
	textureDescriptor.sampleCount = 1;

	GpuTextureSizeAlign sizeAlign = gpuTextureSizeAlign(&textureDescriptor, NULL);
	void* gpuTextureBuffer = gpuMalloc(sizeAlign.size + 1024, sizeAlign.align, GPU_MEMORY_GPU, NULL);
	gpuCreateTexture(&textureDescriptor, gpuTextureBuffer, NULL);

	/* TODO: Update the texture data with the buffer in the bump allocator */

	gpuDeinit();
	return 0;
}

