#ifndef CMN_COMMONARENA_H
#define CMN_COMMONARENA_H

#include <lib/common/common.h>
#include <lib/common/allocator.h>
#include <lib/common/atomic.h>
#include <lib/common/mutex.h>

/**
	Linear allocator over a pre-allocated memory region.
*/
typedef struct {
	/** The backing memory used by the arena. */
	uint8_t*	backing;
	/** The size in bytes of the backing memory. */
	size_t		backingSize;
	/** The currently used bytes in the backing memory. */
	size_t		used;
	/** Mutex used by operations requiring synchronization. */
	CmnMutex	mutex;
} CmnArena;

/**
	Temporary snapshot of a CmnArena allocation state.
	@see CmnArena
*/
typedef struct {
	/** The target arena associated with this state. */
	CmnArena*	arena;
	/** The `used` value captured at begin time. */
	size_t		originalUsed;
} CmnArenaState;

/**
	Creates a CmnArena on top of existing memory.

	@param backingMemory The backing memory start address.
	@param backingMemorySize The backing memory size in bytes.
	@param clearBackingMemory If true, clears the backing memory during initialization.

	@return The initialized arena value.
	@relates CmnArena
*/
CmnArena cmnCreateArena(uint8_t* backingMemory, size_t backingMemorySize, bool clearBackingMemory);

/**
	Allocates `count` objects of type `T` from the arena.

	@param arena The arena to allocate from.
	@param count The number of objects to allocate.
	@param align The requested memory alignment.
	@param[out] result The result of the operation.

	@return The allocated memory, or `nullptr` on failure.
	@retval CMN_SUCCESS Allocation completed successfully.
	@retval CMN_OUT_OF_MEMORY Not enough space was available in the arena backing memory.
	@relates CmnArena
*/
template <typename T> T* cmnArenaAlloc(CmnArena* arena, size_t count, size_t align, CmnResult* result);

/**
	Allocates `count` objects of type `T` from the arena with default alignment.

	@param arena The arena to allocate from.
	@param count The number of objects to allocate.
	@param[out] result The result of the operation.

	@return The allocated memory, or `nullptr` on failure.
	@retval CMN_SUCCESS Allocation completed successfully.
	@retval CMN_OUT_OF_MEMORY Not enough space was available in the arena backing memory.
	@relates CmnArena
*/
template <typename T> T* cmnArenaAlloc(CmnArena* arena, size_t count, CmnResult* result) {
	return cmnArenaAlloc<T>(arena, count, 0, result);
}

/**
	Allocates one object of type `T` from the arena with default alignment.

	@param arena The arena to allocate from.
	@param[out] result The result of the operation.

	@return The allocated memory, or `nullptr` on failure.
	@retval CMN_SUCCESS Allocation completed successfully.
	@retval CMN_OUT_OF_MEMORY Not enough space was available in the arena backing memory.
	@relates CmnArena
*/
template <typename T> T* cmnArenaAlloc(CmnArena* arena, CmnResult* result) {
	return cmnArenaAlloc<T>(arena, 1, 0, result);
}

/**
	Resets the arena usage to zero.

	@param arena The arena to reset.
	@relates CmnArena
*/
void cmnArenaFreeAll(CmnArena* arena);

/**
	Creates a CmnAllocator adapter for a CmnArena.

	@param arena The arena to wrap.

	@return An allocator that allocates from `arena`.
	@relates CmnArena
*/
CmnAllocator cmnArenaAllocator(CmnArena* arena);

/**
	Begins a temporary arena scope.

	@param arena The arena to snapshot.

	@return The saved arena state.
	@relates CmnArenaState
*/
CmnArenaState cmnBeginArenaTemp(CmnArena* arena);

/**
	Ends a temporary arena scope, restoring the saved allocation state.

	@param state The previously captured state.
	@relates CmnArenaState
*/
void cmnEndArenaTemp(CmnArenaState state);

/**
	RAII guard that creates and restores a temporary arena scope.
*/
typedef class CmnArenaTempGuard {
public:
	/** The captured state restored on destruction. */
	CmnArenaState state;

	/**
		Begins a temporary scope for the provided arena.

		@param arena The arena to snapshot.
	*/
	CmnArenaTempGuard(CmnArena* arena) {
		this->state = cmnBeginArenaTemp(arena);
	}

	/**
		Restores the arena state captured at construction.
	*/
	~CmnArenaTempGuard() {
		cmnEndArenaTemp(this->state);
	}
} CmnArenaTempGuard;

#include "arena.inc"

#endif // CMN_COMMONARENA_H
