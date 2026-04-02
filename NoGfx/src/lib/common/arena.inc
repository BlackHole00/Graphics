#include <string.h>

inline GpuArena gpuCreateArena(uint8_t* backing_memory, size_t backing_memory_size) {
	memset(backing_memory, 0, backing_memory_size);

	return {
		/*backing=*/		backing_memory,
		/*backing_size=*/	backing_memory_size,
		/*used=*/		0,
	};
}

// Align must be a power of 2
template <class T>
T* gpuArenaAlloc(GpuArena* arena, size_t count, size_t align) {
	if (align == 0) {
		align = alignof(T);
	}

	size_t current = arena->used;
	uintptr_t base = (uintptr_t)(arena->backing + current);

	uintptr_t aligned = (base + (align - 1)) & ~(uintptr_t)(align - 1);
	size_t padding = aligned - base;

	size_t total_size = padding + sizeof(T) * count;

	if (arena->used + total_size > arena->backing_size) {
		return nullptr;
	}

	arena->used += total_size;

	return (T*)aligned;
}

inline void gpuArenaFreeAll(GpuArena* arena) {
	memset(arena->backing, 0, arena->used);
	arena->used = 0;
}

inline GpuArenaState gpuBeginArenaTemp(GpuArena* arena) {
	return {
		/*arena=*/arena,
		/*original_used=*/arena->used,
	};
}

inline void gpuEndArenaTemp(GpuArenaState state) {
	memset(state.arena->backing + state.original_used, 0, state.arena->used - state.original_used);
	state.arena->used = state.original_used;
}

