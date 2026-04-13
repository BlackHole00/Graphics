#ifndef CMN_POOLALLOCATOR_H
#define CMN_POOLALLOCATOR_H

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <lib/common/common.h>
#include <lib/common/allocator.h>
#include <lib/common/mutex.h>

typedef struct CmnPoolBlockHeader {
	bool		initialized	: 1;
	uint32_t	nextFreeBlock	: 31; 
} CmnPoolBlockHeader;

typedef struct CmnPool {
	uint8_t*		backing;

	size_t			blockCount;
	size_t			blockSize;

	CmnPoolBlockHeader*	firstFree;

	CmnMutex		mutex;
} CmnPool;

CmnPool cmnCreatePool(uint8_t* backingMemory, size_t backingMemorySize, size_t blockSize, size_t prePreparedBlocks);
void* cmnPoolAllocRaw(CmnPool* pool, CmnResult* result);
template<typename T> T* cmnPoolAlloc(CmnPool* pool, CmnResult* result);
void cmnPoolFree(CmnPool* pool, void* address);
CmnAllocator cmnPoolAllocator(CmnPool* pool);

template<typename T>
T* cmnPoolAlloc(CmnPool* pool, CmnResult* result) {
	assert(sizeof(T) <= pool->blockSize);
	return (T*)cmnPoolAllocRaw(pool, result);
}

#endif // CMN_POOLALLOCATOR_H

