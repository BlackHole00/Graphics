#ifndef CMN_COMMONARENA_H
#define CMN_COMMONARENA_H

#include <lib/common/common.h>
#include <lib/common/allocator.h>
#include <lib/common/atomic.h>
#include <lib/common/mutex.h>

// Linear allocator over a pre-allocated memory region.
typedef struct {
	uint8_t*	backing;
	size_t		backingSize;
	// Number of bytes currently consumed.
	size_t		used;
	CmnMutex	mutex;
} CmnArena;

// Snapshot of arena allocation state for temporary allocations.
typedef struct {
	CmnArena*	arena;
	size_t		originalUsed;
} CmnArenaState;

// cmnCreateArena initializes an arena over existing memory.
//
// Inputs:
// - backingMemory: Start address of backing memory.
// - backingMemorySize: Backing memory size in bytes.
// - clearBackingMemory: Clear backing memory during initialization.
//
// Returns:
// - Initialized arena value.
CmnArena cmnCreateArena(uint8_t* backingMemory, size_t backingMemorySize, bool clearBackingMemory);

// cmnArenaAlloc allocates count objects of type T from the arena.
//
// Inputs:
// - arena: Arena to allocate from.
// - count: Number of objects.
// - align: Requested alignment in bytes.
// - result: Optional operation result.
//
// Results:
// - CMN_SUCCESS: Allocation succeeded.
// - CMN_OUT_OF_MEMORY: Arena does not have enough remaining space.
//
// Returns:
// - Allocated memory, or nullptr on failure.
template <typename T> T* cmnArenaAlloc(CmnArena* arena, size_t count, size_t align, CmnResult* result);

// cmnArenaAlloc allocates count objects of type T with default alignment.
//
// Inputs:
// - arena: Arena to allocate from.
// - count: Number of objects.
// - result: Optional operation result.
//
// Results:
// - CMN_SUCCESS: Allocation succeeded.
// - CMN_OUT_OF_MEMORY: Arena does not have enough remaining space.
//
// Returns:
// - Allocated memory, or nullptr on failure.
template <typename T> T* cmnArenaAlloc(CmnArena* arena, size_t count, CmnResult* result) {
	return cmnArenaAlloc<T>(arena, count, 0, result);
}

// cmnArenaAlloc allocates one object of type T with default alignment.
//
// Inputs:
// - arena: Arena to allocate from.
// - result: Optional operation result.
//
// Results:
// - CMN_SUCCESS: Allocation succeeded.
// - CMN_OUT_OF_MEMORY: Arena does not have enough remaining space.
//
// Returns:
// - Allocated memory, or nullptr on failure.
template <typename T> T* cmnArenaAlloc(CmnArena* arena, CmnResult* result) {
	return cmnArenaAlloc<T>(arena, 1, 0, result);
}

// cmnArenaFreeAll resets the arena usage to zero.
//
// Inputs:
// - arena: Arena to reset.
void cmnArenaFreeAll(CmnArena* arena);

// cmnArenaAllocator creates a CmnAllocator adapter for a CmnArena.
//
// Inputs:
// - arena: Arena to wrap.
//
// Returns:
// - Allocator backed by arena.
CmnAllocator cmnArenaAllocator(CmnArena* arena);

// cmnBeginArenaTemp begins a temporary arena scope.
//
// Inputs:
// - arena: Arena to snapshot.
//
// Returns:
// - Saved arena state.
CmnArenaState cmnBeginArenaTemp(CmnArena* arena);

// cmnEndArenaTemp restores arena state captured by cmnBeginArenaTemp.
//
// Inputs:
// - state: Previously captured arena state.
void cmnEndArenaTemp(CmnArenaState state);

// RAII guard that creates and restores a temporary arena scope.
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
