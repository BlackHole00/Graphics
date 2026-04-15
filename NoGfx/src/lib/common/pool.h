#ifndef CMN_POOLALLOCATOR_H
#define CMN_POOLALLOCATOR_H

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <lib/common/common.h>
#include <lib/common/allocator.h>
#include <lib/common/mutex.h>

/**
	Metadata header stored at the beginning of each pool block.
	@see CmnPool
*/
typedef struct CmnPoolBlockHeader {
	/** Whether this block has been initialized at least once. */
	bool		initialized	: 1;
	/** Index of the next free block in the freelist. */
	uint32_t	nextFreeBlock	: 31; 
} CmnPoolBlockHeader;

/**
	Fixed-size block memory pool allocator.
*/
typedef struct CmnPool {
	/** The start address of the pool backing memory. */
	uint8_t*		backing;

	/** Number of blocks available in the backing memory. */
	size_t			blockCount;
	/** Size in bytes of each block. */
	size_t			blockSize;

	/** Head of the free-list. */
	CmnPoolBlockHeader*	firstFree;

	/** Mutex protecting pool operations requiring synchronization. */
	CmnMutex		mutex;
} CmnPool;

/**
	Creates a pool over an existing memory region.

	@param backingMemory The backing memory start address.
	@param backingMemorySize The backing memory size in bytes.
	@param blockSize The size in bytes of each block.
	@param prePreparedBlocks Number of blocks to pre-prepare during initialization.

	@return The initialized pool.
	@relates CmnPool
*/
CmnPool cmnCreatePool(uint8_t* backingMemory, size_t backingMemorySize, size_t blockSize, size_t prePreparedBlocks);

/**
	Allocates one raw block from the pool.

	@param pool The pool to allocate from.
	@param[out] result The result of the operation.

	@return Pointer to the allocated block, or `nullptr` on failure.
	@retval CMN_SUCCESS Allocation completed successfully.
	@retval CMN_OUT_OF_MEMORY No free block was available in the pool.
	@relates CmnPool
*/
void* cmnPoolAllocRaw(CmnPool* pool, CmnResult* result);

/**
	Allocates one object of type `T` from the pool.

	@param pool The pool to allocate from.
	@param[out] result The result of the operation.

	@return Pointer to the allocated object, or `nullptr` on failure.
	@retval CMN_SUCCESS Allocation completed successfully.
	@retval CMN_OUT_OF_MEMORY No free block was available in the pool.
	@relates CmnPool
*/
template<typename T> T* cmnPoolAlloc(CmnPool* pool, CmnResult* result);

/**
	Returns a block to the pool.

	@param pool The pool owning the block.
	@param address The block address to free.
	@relates CmnPool
*/
void cmnPoolFree(CmnPool* pool, void* address);

/**
	Creates a CmnAllocator adapter for a CmnPool.

	@param pool The pool to wrap.

	@return An allocator that allocates from `pool`.
	@relates CmnPool
*/
CmnAllocator cmnPoolAllocator(CmnPool* pool);

/**
	Allocates one object of type `T` from the pool.

	@param pool The pool to allocate from.
	@param[out] result The result of the operation.

	@return Pointer to the allocated object, or `nullptr` on failure.
	@retval CMN_SUCCESS Allocation completed successfully.
	@retval CMN_OUT_OF_MEMORY No free block was available in the pool.
	@relates CmnPool
*/
template<typename T>
T* cmnPoolAlloc(CmnPool* pool, CmnResult* result) {
	assert(sizeof(T) <= pool->blockSize);
	return (T*)cmnPoolAllocRaw(pool, result);
}

#endif // CMN_POOLALLOCATOR_H

