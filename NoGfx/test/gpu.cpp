#include "test.h"

#include <gpu/gpu.h>

GpuBackend selectBackendForCurrentPlatform(void) {
	#ifdef __APPLE__
		return GPU_METAL_4;
	#else
		return GPU_NONE;
	#endif
}

GpuBackend selectUnavailableBackendForCurrentPlatform(void) {
	#ifdef __APPLE__
		return GPU_VULKAN;
	#else
		return GPU_METAL_4;
	#endif
}

GpuTextureDesc makeDefaultTextureDesc(void) {
	GpuTextureDesc desc = {};
	desc.type = GPU_TEXTURE_2D;
	desc.dimensions[0] = 64;
	desc.dimensions[1] = 64;
	desc.dimensions[2] = 1;
	desc.mipCount = 1;
	desc.layerCount = 1;
	desc.sampleCount = 1;
	desc.format = GPU_FORMAT_RGBA8_UNORM;
	desc.usage = GPU_USAGE_SAMPLED;

	return desc;
}

bool initGpuAndSelectFirstDevice(GpuResult* result) {
	GpuInitDesc initDesc = {};
	initDesc.backend = selectBackendForCurrentPlatform();
	initDesc.validationEnabled = true;

	gpuInit(&initDesc, result);
	if (*result != GPU_SUCCESS) {
		return false;
	}

	GpuDeviceInfo* devices = nullptr;
	size_t count = 0;
	gpuEnumerateDevices(&devices, &count, result);
	if (*result != GPU_SUCCESS || count == 0 || devices == nullptr) {
		gpuDeinit();
		return false;
	}

	gpuSelectDevice(devices[0].identifier, result);
	if (*result != GPU_SUCCESS) {
		gpuDeinit();
		return false;
	}

	return true;
}

void checkGpuInitAndDeinit(Test* test) {
	GpuInitDesc desc = {};
	desc.backend = selectBackendForCurrentPlatform();
	desc.validationEnabled = true;

	GpuResult result;
	gpuInit(&desc, &result);

	TEST_ASSERT(test, result == GPU_SUCCESS);

	gpuDeinit();
}

void checkGpuInvalidBackend(Test* test) {
	GpuInitDesc desc = {};
	desc.backend = selectUnavailableBackendForCurrentPlatform();
	desc.validationEnabled = true;

	GpuResult result;
	gpuInit(&desc, &result);

	TEST_ASSERT(test, result == GPU_BACKEND_NOT_SUPPORTED);

	gpuDeinit();
}

void checkGpuEnumerateDevices(Test* test) {
	GpuInitDesc desc = {};
	desc.backend = selectBackendForCurrentPlatform();
	desc.validationEnabled = true;

	GpuResult result;
	gpuInit(&desc, &result);
	TEST_ASSERT(test, result == GPU_SUCCESS);

	GpuDeviceInfo* devices = nullptr;
	size_t count = 0;

	gpuEnumerateDevices(&devices, &count, &result);

	TEST_ASSERT(test, result == GPU_SUCCESS);
	TEST_ASSERT(test, count > 0);
	TEST_ASSERT(test, devices != nullptr);

	gpuDeinit();
}

void checkGpuSelectDevice(Test* test) {
	GpuInitDesc desc = {};
	desc.backend = selectBackendForCurrentPlatform();
	desc.validationEnabled = true;

	GpuResult result;
	gpuInit(&desc, &result);
	TEST_ASSERT(test, result == GPU_SUCCESS);

	GpuDeviceInfo* devices = nullptr;
	size_t count = 0;

	gpuEnumerateDevices(&devices, &count, &result);
	if (count == 0) {
		return;
	}

	gpuSelectDevice(devices[0].identifier, &result);

	TEST_ASSERT(test, result == GPU_SUCCESS);

	gpuDeinit();
}

void checkGpuSelectInvalidDevice(Test* test) {
	GpuInitDesc desc = {};
	desc.backend = selectBackendForCurrentPlatform();
	desc.validationEnabled = true;

	GpuResult result;
	gpuInit(&desc, &result);
	TEST_ASSERT(test, result == GPU_SUCCESS);

	gpuSelectDevice((GpuDeviceId)999999, &result);
	TEST_ASSERT(test, result == GPU_INVALID_DEVICE);

	gpuDeinit();
}

void checkGpuDoubleDeviceSelection(Test* test) {
	GpuInitDesc desc = {};
	desc.backend = selectBackendForCurrentPlatform();
	desc.validationEnabled = true;

	GpuResult result;
	gpuInit(&desc, &result);
	TEST_ASSERT(test, result == GPU_SUCCESS);

	GpuDeviceInfo* devices = nullptr;
	size_t count = 0;
	gpuEnumerateDevices(&devices, &count, &result);
	if (count == 0) {
		return;
	}

	gpuSelectDevice(devices[0].identifier, &result);
	TEST_ASSERT(test, result == GPU_SUCCESS);

	gpuSelectDevice(devices[0].identifier, &result);
	TEST_ASSERT(test, result == GPU_DEVICE_ALREADY_SELECTED);

	gpuDeinit();
}

void checkGpuMallocAndFree(Test* test) {
	GpuInitDesc desc = {};
	desc.backend = selectBackendForCurrentPlatform();
	desc.validationEnabled = true;

	GpuResult result;
	gpuInit(&desc, &result);
	TEST_ASSERT(test, result == GPU_SUCCESS);

	GpuDeviceInfo* devices = nullptr;
	size_t count = 0;
	gpuEnumerateDevices(&devices, &count, &result);
	if (count == 0) {
		return;
	}

	gpuSelectDevice(devices[0].identifier, &result);

	void* ptr = gpuMalloc(1024, 16, GPU_MEMORY_DEFAULT, &result);

	TEST_ASSERT(test, result == GPU_SUCCESS);
	TEST_ASSERT(test, ptr != nullptr);

	gpuFree(ptr);

	gpuDeinit();
}

void checkGpuMallocAndFreeGpuMemory(Test* test) {
	GpuInitDesc desc = {};
	desc.backend = selectBackendForCurrentPlatform();
	desc.validationEnabled = true;

	GpuResult result;
	gpuInit(&desc, &result);
	TEST_ASSERT(test, result == GPU_SUCCESS);

	GpuDeviceInfo* devices = nullptr;
	size_t count = 0;
	gpuEnumerateDevices(&devices, &count, &result);
	if (count == 0) {
		return;
	}

	gpuSelectDevice(devices[0].identifier, &result);

	void* ptr = gpuMalloc(1024, 16, GPU_MEMORY_GPU, &result);

	TEST_ASSERT(test, result == GPU_SUCCESS);
	TEST_ASSERT(test, ptr != nullptr);

	gpuFree(ptr);

	gpuDeinit();
}

void checkGpuHostToDevicePointerOnGpuMemory(Test* test) {
	GpuInitDesc desc = {};
	desc.backend = selectBackendForCurrentPlatform();
	desc.validationEnabled = true;

	GpuResult result;
	gpuInit(&desc, &result);
	TEST_ASSERT(test, result == GPU_SUCCESS);

	GpuDeviceInfo* devices = nullptr;
	size_t count = 0;
	gpuEnumerateDevices(&devices, &count, &result);
	if (count == 0) {
		return;
	}

	gpuSelectDevice(devices[0].identifier, &result);

	void* ptr = gpuMalloc(256, 16, GPU_MEMORY_GPU, &result);
	TEST_ASSERT(test, result == GPU_SUCCESS);
	TEST_ASSERT(test, ptr != nullptr);

	void* devicePtr = gpuHostToDevicePointer(ptr, &result);

	TEST_ASSERT(test, result == GPU_ALLOCATION_MEMORY_IS_GPU);
	TEST_ASSERT(test, devicePtr == nullptr);

	gpuFree(ptr);

	gpuDeinit();
}

void checkGpuFreeInvalidPointer(Test* test) {
	(void)test;

	GpuInitDesc desc = {};
	desc.backend = selectBackendForCurrentPlatform();
	desc.validationEnabled = true;

	GpuResult result;
	gpuInit(&desc, &result);
	TEST_ASSERT(test, result == GPU_SUCCESS);

	GpuDeviceInfo* devices = nullptr;
	size_t count = 0;
	gpuEnumerateDevices(&devices, &count, &result);
	if (count == 0) {
		return;
	}

	gpuSelectDevice(devices[0].identifier, &result);

	int dummy;
	gpuFree(&dummy); // Should not crash

	gpuDeinit();
}

void checkGpuHostToDevicePointer(Test* test) {
	GpuInitDesc desc = {};
	desc.backend = selectBackendForCurrentPlatform();
	desc.validationEnabled = true;

	GpuResult result;
	gpuInit(&desc, &result);
	TEST_ASSERT(test, result == GPU_SUCCESS);

	GpuDeviceInfo* devices = nullptr;
	size_t count = 0;
	gpuEnumerateDevices(&devices, &count, &result);
	if (count == 0) {
		return;
	}

	gpuSelectDevice(devices[0].identifier, &result);

	void* ptr = gpuMalloc(256, 16, GPU_MEMORY_DEFAULT, &result);
	if (ptr == nullptr) {
		return;
	}

	void* devicePtr = gpuHostToDevicePointer(ptr, &result);

	TEST_ASSERT(test, result == GPU_SUCCESS);
	TEST_ASSERT(test, devicePtr != nullptr);

	gpuFree(ptr);

	gpuDeinit();
}

void checkGpuHostToDevicePointerWithOffset(Test* test) {
	GpuInitDesc desc = {};
	desc.backend = selectBackendForCurrentPlatform();
	desc.validationEnabled = true;

	GpuResult result;
	gpuInit(&desc, &result);
	TEST_ASSERT(test, result == GPU_SUCCESS);

	GpuDeviceInfo* devices = nullptr;
	size_t count = 0;
	gpuEnumerateDevices(&devices, &count, &result);
	if (count == 0) {
		return;
	}

	gpuSelectDevice(devices[0].identifier, &result);

	void* ptr = gpuMalloc(256, 16, GPU_MEMORY_DEFAULT, &result);
	if (ptr == nullptr) {
		return;
	}

	void* baseDevicePtr = gpuHostToDevicePointer(ptr, &result);
	TEST_ASSERT(test, result == GPU_SUCCESS);
	TEST_ASSERT(test, baseDevicePtr != nullptr);

	ptr = (void*)((uintptr_t)ptr + 128);
	void* devicePtrWithOffset = gpuHostToDevicePointer(ptr, &result);
	TEST_ASSERT(test, result == GPU_SUCCESS);
	TEST_ASSERT(test, baseDevicePtr != nullptr);

	uintptr_t offset = (uintptr_t)devicePtrWithOffset - (uintptr_t)baseDevicePtr;
	TEST_ASSERT(test, offset = 128);

	gpuFree(ptr);

	gpuDeinit();
}

void checkGpuTextureSizeAlign(Test* test) {
	GpuResult result;
	if (!initGpuAndSelectFirstDevice(&result)) {
		TEST_ASSERT(test, result == GPU_SUCCESS);
		return;
	}

	GpuTextureDesc desc = makeDefaultTextureDesc();
	GpuTextureSizeAlign sizeAlign = gpuTextureSizeAlign(&desc, &result);

	TEST_ASSERT(test, result == GPU_SUCCESS);
	TEST_ASSERT(test, sizeAlign.size > 0);
	TEST_ASSERT(test, sizeAlign.align > 0);

	gpuDeinit();
}

void checkGpuTextureSizeAlignInvalidDesc(Test* test) {
	GpuResult result;
	if (!initGpuAndSelectFirstDevice(&result)) {
		TEST_ASSERT(test, result == GPU_SUCCESS);
		return;
	}

	GpuTextureSizeAlign sizeAlign = gpuTextureSizeAlign(nullptr, &result);

	TEST_ASSERT(test, result == GPU_INVALID_PARAMETERS);
	TEST_ASSERT(test, sizeAlign.size == 0);
	TEST_ASSERT(test, sizeAlign.align == 0);

	gpuDeinit();
}

void checkGpuCreateTexture(Test* test) {
	GpuResult result;
	if (!initGpuAndSelectFirstDevice(&result)) {
		TEST_ASSERT(test, result == GPU_SUCCESS);
		return;
	}

	GpuTextureDesc desc = makeDefaultTextureDesc();
	GpuTextureSizeAlign sizeAlign = gpuTextureSizeAlign(&desc, &result);
	TEST_ASSERT(test, result == GPU_SUCCESS);
	TEST_ASSERT(test, sizeAlign.size > 0);

	void* ptrGpu = gpuMalloc(sizeAlign.size, sizeAlign.align, GPU_MEMORY_GPU, &result);
	TEST_ASSERT(test, result == GPU_SUCCESS);
	TEST_ASSERT(test, ptrGpu != nullptr);

	GpuTexture texture = gpuCreateTexture(&desc, ptrGpu, &result);

	TEST_ASSERT(test, result == GPU_SUCCESS);
	TEST_ASSERT(test, texture != 0);

	gpuFree(ptrGpu);
	gpuDeinit();
}

void checkGpuCreateTextureOnCpuAllocation(Test* test) {
	GpuResult result;
	if (!initGpuAndSelectFirstDevice(&result)) {
		TEST_ASSERT(test, result == GPU_SUCCESS);
		return;
	}

	GpuTextureDesc desc = makeDefaultTextureDesc();
	GpuTextureSizeAlign sizeAlign = gpuTextureSizeAlign(&desc, &result);
	TEST_ASSERT(test, result == GPU_SUCCESS);
	TEST_ASSERT(test, sizeAlign.size > 0);

	void* ptrCpu = gpuMalloc(sizeAlign.size, sizeAlign.align, GPU_MEMORY_DEFAULT, &result);
	TEST_ASSERT(test, result == GPU_SUCCESS);
	TEST_ASSERT(test, ptrCpu != nullptr);

	GpuTexture texture = gpuCreateTexture(&desc, ptrCpu, &result);

	TEST_ASSERT(test, result == GPU_ALLOCATION_MEMORY_IS_CPU);
	TEST_ASSERT(test, texture == 0);

	gpuFree(ptrCpu);
	gpuDeinit();
}

void checkGpuCreateTextureInvalidDesc(Test* test) {
	GpuResult result;
	if (!initGpuAndSelectFirstDevice(&result)) {
		TEST_ASSERT(test, result == GPU_SUCCESS);
		return;
	}

	void* ptrGpu = gpuMalloc(1024, 16, GPU_MEMORY_GPU, &result);
	TEST_ASSERT(test, result == GPU_SUCCESS);
	TEST_ASSERT(test, ptrGpu != nullptr);

	GpuTexture texture = gpuCreateTexture(nullptr, ptrGpu, &result);

	TEST_ASSERT(test, result == GPU_INVALID_PARAMETERS);
	TEST_ASSERT(test, texture == 0);

	gpuFree(ptrGpu);
	gpuDeinit();
}

void checkGpuTextureViewDescriptor(Test* test) {
	GpuResult result;
	if (!initGpuAndSelectFirstDevice(&result)) {
		TEST_ASSERT(test, result == GPU_SUCCESS);
		return;
	}

	GpuTextureDesc textureDesc = makeDefaultTextureDesc();
	GpuTextureSizeAlign sizeAlign = gpuTextureSizeAlign(&textureDesc, &result);
	TEST_ASSERT(test, result == GPU_SUCCESS);

	void* ptrGpu = gpuMalloc(sizeAlign.size, sizeAlign.align, GPU_MEMORY_GPU, &result);
	TEST_ASSERT(test, result == GPU_SUCCESS);

	GpuTexture texture = gpuCreateTexture(&textureDesc, ptrGpu, &result);
	TEST_ASSERT(test, result == GPU_SUCCESS);
	TEST_ASSERT(test, texture != 0);

	GpuViewDesc viewDesc = {};
	viewDesc.format = GPU_FORMAT_RGBA8_UNORM;
	viewDesc.baseMip = 0;
	viewDesc.mipCount = 1;
	viewDesc.baseLayer = 0;
	viewDesc.layerCount = 1;

	GpuTextureDescriptor descriptor = gpuTextureViewDescriptor(texture, &viewDesc, &result);

	TEST_ASSERT(test, result == GPU_SUCCESS);
	TEST_ASSERT(test, descriptor.data[0] != 0);

	gpuFree(ptrGpu);
	gpuDeinit();
}

void checkGpuRWTextureViewDescriptor(Test* test) {
	GpuResult result;
	if (!initGpuAndSelectFirstDevice(&result)) {
		TEST_ASSERT(test, result == GPU_SUCCESS);
		return;
	}

	GpuTextureDesc textureDesc = makeDefaultTextureDesc();
	GpuTextureSizeAlign sizeAlign = gpuTextureSizeAlign(&textureDesc, &result);
	TEST_ASSERT(test, result == GPU_SUCCESS);

	void* ptrGpu = gpuMalloc(sizeAlign.size, sizeAlign.align, GPU_MEMORY_GPU, &result);
	TEST_ASSERT(test, result == GPU_SUCCESS);

	GpuTexture texture = gpuCreateTexture(&textureDesc, ptrGpu, &result);
	TEST_ASSERT(test, result == GPU_SUCCESS);
	TEST_ASSERT(test, texture != 0);

	GpuViewDesc viewDesc = {};
	viewDesc.format = GPU_FORMAT_RGBA8_UNORM;
	viewDesc.baseMip = 0;
	viewDesc.mipCount = 1;
	viewDesc.baseLayer = 0;
	viewDesc.layerCount = 1;

	GpuTextureDescriptor descriptor = gpuRWTextureViewDescriptor(texture, &viewDesc, &result);

	TEST_ASSERT(test, result == GPU_SUCCESS);
	TEST_ASSERT(test, descriptor.data[0] != 0);

	gpuFree(ptrGpu);
	gpuDeinit();
}

void checkGpuTextureViewDescriptorInvalidTexture(Test* test) {
	GpuResult result;
	if (!initGpuAndSelectFirstDevice(&result)) {
		TEST_ASSERT(test, result == GPU_SUCCESS);
		return;
	}

	GpuViewDesc viewDesc = {};
	viewDesc.format = GPU_FORMAT_RGBA8_UNORM;
	viewDesc.baseMip = 0;
	viewDesc.mipCount = 1;
	viewDesc.baseLayer = 0;
	viewDesc.layerCount = 1;

	GpuTextureDescriptor descriptor = gpuTextureViewDescriptor((GpuTexture)0, &viewDesc, &result);

	TEST_ASSERT(test, result == GPU_NO_SUCH_TEXTURE_FOUND);
	TEST_ASSERT(test, descriptor.data[0] == 0);

	gpuDeinit();
}

void checkGpuTextureViewDescriptorInvalidDesc(Test* test) {
	GpuResult result;
	if (!initGpuAndSelectFirstDevice(&result)) {
		TEST_ASSERT(test, result == GPU_SUCCESS);
		return;
	}

	GpuTextureDesc textureDesc = makeDefaultTextureDesc();
	GpuTextureSizeAlign sizeAlign = gpuTextureSizeAlign(&textureDesc, &result);
	TEST_ASSERT(test, result == GPU_SUCCESS);

	void* ptrGpu = gpuMalloc(sizeAlign.size, sizeAlign.align, GPU_MEMORY_GPU, &result);
	TEST_ASSERT(test, result == GPU_SUCCESS);

	GpuTexture texture = gpuCreateTexture(&textureDesc, ptrGpu, &result);
	TEST_ASSERT(test, result == GPU_SUCCESS);
	TEST_ASSERT(test, texture != 0);

	GpuTextureDescriptor descriptor = gpuTextureViewDescriptor(texture, nullptr, &result);

	TEST_ASSERT(test, result == GPU_INVALID_PARAMETERS);
	TEST_ASSERT(test, descriptor.data[0] == 0);

	gpuFree(ptrGpu);
	gpuDeinit();
}

