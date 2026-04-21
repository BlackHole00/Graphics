void checkGpuConcurrentAllocationStressOnCpuMemory(Test* test) {
	runGpuAllocationStress(test, GPU_MEMORY_DEFAULT, true);
}

void checkGpuConcurrentAllocationStressOnGpuMemory(Test* test) {
	runGpuAllocationStress(test, GPU_MEMORY_GPU, false);
}

void checkGpuConcurrentHostPointerStress(Test* test) {
	runGpuHostPointerStress(test);
}

void checkGpuConcurrentTextureStress(Test* test) {
	runGpuTextureStress(test);
}

void checkGpuConcurrentPipelineStress(Test* test) {
	runGpuPipelineStress(test);
}

void checkGpuDeferredAllocationDeletionThresholdFlush(Test* test) {
	GpuResult result;
	if (!initGpuAndSelectFirstDevice(&result)) {
		TEST_ASSERT(test, result == GPU_SUCCESS);
		return;
	}

	const size_t allocationCount = 192;
	const size_t allocationSize = 64 * 1024;

	void** allocations = (void**)malloc(sizeof(void*) * allocationCount);
	if (allocations == nullptr) {
		testOutOfMemory(test);
	}

	for (size_t i = 0; i < allocationCount; i++) {
		allocations[i] = gpuMalloc(allocationSize, 16, GPU_MEMORY_DEFAULT, &result);
		TEST_ASSERT(test, result == GPU_SUCCESS);
		TEST_ASSERT(test, allocations[i] != nullptr);
	}

	void* first = allocations[0];
	for (size_t i = 0; i < allocationCount; i++) {
		gpuFree(allocations[i]);
	}

	void* devicePtr = gpuHostToDevicePointer(first, &result);
	TEST_ASSERT(test, result == GPU_NO_SUCH_ALLOCATION_FOUND);
	TEST_ASSERT(test, devicePtr == nullptr);

	free(allocations);
	gpuDeinit();
}

void checkGpuDeferredTextureDeletionThresholdFlush(Test* test) {
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

	const size_t textureCount = 136;

	void** allocations = (void**)malloc(sizeof(void*) * textureCount);
	GpuTexture* textures = (GpuTexture*)malloc(sizeof(GpuTexture) * textureCount);
	if (allocations == nullptr || textures == nullptr) {
		testOutOfMemory(test);
	}

	for (size_t i = 0; i < textureCount; i++) {
		allocations[i] = gpuMalloc(sizeAlign.size, sizeAlign.align, GPU_MEMORY_GPU, &result);
		TEST_ASSERT(test, result == GPU_SUCCESS);
		TEST_ASSERT(test, allocations[i] != nullptr);

		textures[i] = gpuCreateTexture(&desc, allocations[i], &result);
		TEST_ASSERT(test, result == GPU_SUCCESS);
		TEST_ASSERT(test, textures[i] != 0);
	}

	GpuTexture firstTexture = textures[0];
	for (size_t i = 0; i < textureCount; i++) {
		gpuFree(allocations[i]);
	}

	GpuViewDesc viewDesc = {};
	viewDesc.format = desc.format;
	viewDesc.baseMip = 0;
	viewDesc.mipCount = 1;
	viewDesc.baseLayer = 0;
	viewDesc.layerCount = 1;

	GpuTextureDescriptor descriptor = gpuTextureViewDescriptor(firstTexture, &viewDesc, &result);
	TEST_ASSERT(test, result == GPU_NO_SUCH_TEXTURE_FOUND);
	TEST_ASSERT(test, descriptor.data[0] == 0);

	free(allocations);
	free(textures);
	gpuDeinit();
}

