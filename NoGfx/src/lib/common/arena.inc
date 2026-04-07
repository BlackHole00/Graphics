#include <string.h>

inline CmnArena cmnCreateArena(uint8_t* backingMemory, size_t backingMemorySize) {
	memset(backingMemory, 0, backingMemorySize);

	return {
		/*backing=*/		backingMemory,
		/*backingSize=*/	backingMemorySize,
		/*used=*/		0,
	};
}

// Align must be a power of 2
template <class T>
T* cmnArenaAlloc(CmnArena* arena, size_t count, size_t align, CmnResult* result) {
	if (align == 0) {
		align = alignof(T);
	}

	size_t current = arena->used;
	uintptr_t base = (uintptr_t)(arena->backing + current);

	uintptr_t aligned = (base + (align - 1)) & ~(uintptr_t)(align - 1);
	size_t padding = aligned - base;

	size_t totalSize = padding + sizeof(T) * count;

	if (arena->used + totalSize > arena->backingSize) {
		CMN_SET_RESULT(result, CMN_OUT_OF_MEMORY);
		return nullptr;
	}

	arena->used += totalSize;

	CMN_SET_RESULT(result, CMN_SUCCESS);
	return (T*)aligned;
}

inline void cmnArenaFreeAll(CmnArena* arena) {
	memset(arena->backing, 0, arena->used);
	arena->used = 0;
}

inline CmnArenaState cmnBeginArenaTemp(CmnArena* arena) {
	return {
		/*arena=*/arena,
		/*originalUsed=*/arena->used,
	};
}

inline void cmnEndArenaTemp(CmnArenaState state) {
	memset(state.arena->backing + state.originalUsed, 0, state.arena->used - state.originalUsed);
	state.arena->used = state.originalUsed;
}

