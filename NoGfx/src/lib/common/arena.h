#ifndef CMN_COMMONARENA_H
#define CMN_COMMONARENA_H

#include <lib/common/common.h>

typedef struct {
	uint8_t*	backing;
	size_t		backing_size;
	size_t		used;
} CmnArena;

typedef struct {
	CmnArena*	arena;
	size_t		original_used;
} CmnArenaState;

CmnArena cmnCreateArena(uint8_t* backing_memory, size_t backing_memory_size);

template <typename T> T* cmnArenaAlloc(CmnArena* arena, size_t count = 1, size_t align = 0);
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
