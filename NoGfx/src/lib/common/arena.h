#ifndef CMN_COMMONARENA_H
#define CMN_COMMONARENA_H

#include <lib/common/common.h>

typedef struct {
	uint8_t*	backing;
	size_t		backingSize;
	size_t		used;
} CmnArena;

typedef struct {
	CmnArena*	arena;
	size_t		originalUsed;
} CmnArenaState;

CmnArena cmnCreateArena(uint8_t* backingMemory, size_t backingMemorySize, bool clearBackingMemory);

template <typename T> T* cmnArenaAlloc(CmnArena* arena, size_t count, size_t align, CmnResult* result);
template <typename T> T* cmnArenaAlloc(CmnArena* arena, size_t count, CmnResult* result) {
	return cmnArenaAlloc<T>(arena, count, 0, result);
}
template <typename T> T* cmnArenaAlloc(CmnArena* arena, CmnResult* result) {
	return cmnArenaAlloc<T>(arena, 1, 0, result);
}
void cmnArenaFreeAll(CmnArena* arena);

CmnArenaState cmnBeginArenaTemp(CmnArena* arena);
void cmnEndArenaTemp(CmnArenaState state);

typedef class CmnArenaTempGuard {
public:
	CmnArenaState state;

	CmnArenaTempGuard(CmnArena* arena) {
		this->state = cmnBeginArenaTemp(arena);
	}
	~CmnArenaTempGuard() {
		cmnEndArenaTemp(this->state);
	}
} CmnArenaTempGuard;

#include "arena.inc"

#endif // CMN_COMMONARENA_H
