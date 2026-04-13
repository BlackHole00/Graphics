#include "arena.h"

static void* cmnArenaAllocatorAlloc(void* arena, size_t size, size_t align, CmnResult* result) {
	return (void*)cmnArenaAlloc<uint8_t>((CmnArena*)arena, size, align, result);
}

static void* cmnArenaAllocatorRealloc(void*, void*, size_t, size_t, size_t, CmnResult* result) {
	CMN_SET_RESULT(result, CMN_UNSUPPORTED_OPERATION);
	return nullptr;
}

static void cmnArenaAllocatorFree(void*, void*, CmnResult* result) {
	CMN_SET_RESULT(result, CMN_UNSUPPORTED_OPERATION);
}

static void cmnArenaAllocatorFreeAll(void* arena, CmnResult* result) {
	cmnArenaFreeAll((CmnArena*)arena);
	CMN_SET_RESULT(result, CMN_SUCCESS);
}

static CmnAllocatorVTable gCmnArenaAllocatorVTable = {
	/*alloc=*/	cmnArenaAllocatorAlloc,
	/*realloc=*/	cmnArenaAllocatorRealloc,
	/*free=*/	cmnArenaAllocatorFree,
	/*freeAll=*/	cmnArenaAllocatorFreeAll,
};

CmnAllocator cmnArenaAllocator(CmnArena *arena) {
	return CmnAllocator {
		/*vtable=*/	&gCmnArenaAllocatorVTable,
		/*data=*/	arena,
	};
}

