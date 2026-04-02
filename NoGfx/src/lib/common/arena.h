#ifndef GPU_COMMONARENA_H
#define GPU_COMMONARENA_H

#include <lib/common/common.h>

typedef struct {
	uint8_t*	backing;
	size_t		backing_size;
	size_t		used;
} GpuArena;

typedef struct {
	GpuArena*	arena;
	size_t		original_used;
} GpuArenaState;

GpuArena gpuCreateArena(uint8_t* backing_memory, size_t backing_memory_size);

template <typename T> T* gpuArenaAlloc(GpuArena* arena, size_t count = 1, size_t align = 0);
void gpuArenaFreeAll(GpuArena* arena);

GpuArenaState gpuBeginArenaTemp(GpuArena* arena);
void gpuEndArenaTemp(GpuArenaState state);

typedef class GpuArenaTempGuard {
public:
	GpuArenaState state;

	GpuArenaTempGuard(GpuArena* arena) {
		this->state = gpuBeginArenaTemp(arena);
	}
	~GpuArenaTempGuard() {
		gpuEndArenaTemp(this->state);
	}
} GpuArenaTempGuard;

#include "arena.inc"

#endif // GPU_COMMONARENA_H
