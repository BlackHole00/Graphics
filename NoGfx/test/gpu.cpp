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

