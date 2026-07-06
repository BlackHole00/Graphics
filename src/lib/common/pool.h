#ifndef CMN_POOLALLOCATOR_H
#define CMN_POOLALLOCATOR_H

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <lib/common/common.h>
#include <lib/common/allocator.h>
#include <lib/common/mutex.h>

// Metadata header stored at the beginning of each pool block.
typedef struct CmnPoolBlockHeader {
	bool		initialized	: 1;
	uint32_t	nextFreeBlock	: 31; 
} CmnPoolBlockHeader;

// Fixed-size block memory pool allocator.
typedef struct CmnPool {
	uint8_t*		backing;

	// Number of blocks in backing memory.
	size_t			blockCount;
	size_t			blockSize;

	CmnPoolBlockHeader*	firstFree;

	CmnMutex		mutex;
} CmnPool;

// Creates a pool over existing memory.
//
// Inputs:
// - backingMemory: Start address of backing memory.
// - backingMemorySize: Backing memory size in bytes.
// - blockSize: Size in bytes of each block.
// - prePreparedBlocks: Number of blocks to pre-prepare.
//
// Returns:
// - Initialized pool.
CmnPool cmnCreatePool(uint8_t* backingMemory, size_t backingMemorySize, size_t blockSize, size_t prePreparedBlocks);

// Allocates one raw block from the pool.
//
// Inputs:
// - pool: Pool to allocate from.
// - result: Optional operation result.
//
// Results:
// - CMN_SUCCESS: Allocation succeeded.
// - CMN_OUT_OF_MEMORY: No free block is available in the pool.
//
// Returns:
// - Pointer to allocated block, or nullptr on failure.
void* cmnPoolAllocRaw(CmnPool* pool, CmnResult* result);

// Allocates one object of type T from the pool.
//
// Inputs:
// - pool: Pool to allocate from.
// - result: Optional operation result.
//
// Results:
// - CMN_SUCCESS: Allocation succeeded.
// - CMN_OUT_OF_MEMORY: No free block is available in the pool.
//
// Returns:
// - Pointer to allocated object, or nullptr on failure.
template<typename T> T* cmnPoolAlloc(CmnPool* pool, CmnResult* result);

// Returns a block to the pool.
//
// Inputs:
// - pool: Pool owning the block.
// - address: Block address to free.
void cmnPoolFree(CmnPool* pool, void* address);

// Creates a CmnAllocator adapter for a CmnPool.
//
// Inputs:
// - pool: Pool to wrap.
//
// Returns:
// - Allocator backed by pool.
CmnAllocator cmnPoolAllocator(CmnPool* pool);

// Allocates one object of type T from the pool.
//
// Inputs:
// - pool: Pool to allocate from.
// - result: Optional operation result.
//
// Results:
// - CMN_SUCCESS: Allocation succeeded.
// - CMN_OUT_OF_MEMORY: No free block is available in the pool.
//
// Returns:
// - Pointer to allocated object, or nullptr on failure.
template<typename T>
T* cmnPoolAlloc(CmnPool* pool, CmnResult* result) {
	assert(sizeof(T) <= pool->blockSize);
	return (T*)cmnPoolAllocRaw(pool, result);
}

#endif // CMN_POOLALLOCATOR_H

