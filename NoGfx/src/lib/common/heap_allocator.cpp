#include "heap_allocator.h"

#include <stdlib.h>
#include <string.h>

void* cmnHeapAllocRaw(size_t size, size_t align, CmnResult* result) {
	void* data;
	if (align == 0) {
		data = malloc(size);
	} else {
		data = aligned_alloc(align, size);
	}

	if (data == nullptr) {
		CMN_SET_RESULT(result, CMN_OUT_OF_MEMORY);
		return nullptr;
	}

	memset(data, 0, size);

	CMN_SET_RESULT(result, CMN_SUCCESS);
	return data;
}

void* cmnHeapReallocRaw(void* address, size_t oldSize, size_t newSize, size_t align, CmnResult* result) {
	size_t contentSize = (oldSize < newSize) ? oldSize : newSize;
	void* newAddress;

	if (align == 0) {
		newAddress = realloc(address, newSize);
	} else {
		newAddress = aligned_alloc(align, newSize);
		if (newAddress == nullptr) {
			CMN_SET_RESULT(result, CMN_OUT_OF_MEMORY);
			return nullptr;
		}

		memcpy(newAddress, address, contentSize);
		free(address);
	}

	if (newAddress == nullptr) {
		CMN_SET_RESULT(result, CMN_OUT_OF_MEMORY);
		return nullptr;
	}

	if (oldSize < newSize) {
		uintptr_t zeroStart = (uintptr_t)newAddress + contentSize;
		size_t bytesToZero = newSize - oldSize;
		memset((void*)zeroStart, 0, bytesToZero);
	}

	CMN_SET_RESULT(result, CMN_SUCCESS);
	return newAddress;
}

void cmnHeapFree(void* data) {
	free(data);
}

static void* cmnHeapAllocatorAlloc(void*, size_t size, size_t align, CmnResult* result) {
	return cmnHeapAllocRaw(size, align, result);
}

static void* cmnHeapAllocatorRealloc(void*, void* address, size_t oldSize, size_t newSize, size_t align, CmnResult* result) {
	return cmnHeapReallocRaw(address, oldSize, newSize, align, result);
}

static void cmnHeapAllocatorFree(void*, void* address, CmnResult* result) {
	cmnHeapFree(address);
	CMN_SET_RESULT(result, CMN_SUCCESS);
}

static void cmnHeapAllocatorFreeAll(void*, CmnResult* result) {
	CMN_SET_RESULT(result, CMN_UNSUPPORTED_OPERATION);
}

static CmnAllocatorVTable gCmnHeapAllocatorVTable = {
	/*alloc=*/	cmnHeapAllocatorAlloc,
	/*realloc=*/	cmnHeapAllocatorRealloc,
	/*free=*/	cmnHeapAllocatorFree,
	/*freeAll=*/	cmnHeapAllocatorFreeAll,
};

static CmnAllocator gCmnHeapAllocator = {
	/*vtable=*/	&gCmnHeapAllocatorVTable,
	/*data=*/	nullptr,
};

CmnAllocator cmnHeapAllocator(void) {
	return gCmnHeapAllocator;
}


