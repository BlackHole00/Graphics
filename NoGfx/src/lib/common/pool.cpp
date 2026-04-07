#include "pool.h"

static CmnPoolBlockHeader* cmnPoolHeaderAt(CmnPool* pool, size_t index) {
	if (index >= pool->blockCount) {
		return nullptr;
	}

	size_t offset = index * pool->blockSize;
	return (CmnPoolBlockHeader*)&pool->backing[offset];
}

static size_t cmnPoolIndexOfBlockAt(CmnPool* pool, void* address) {
	return ((size_t)address - (size_t)pool->backing) / pool->blockSize;
}

CmnPool cmnCreatePool(uint8_t* backingMemory, size_t backingMemorySize, size_t blockSize, size_t prePreparedBlocks) {
	if (blockSize < sizeof(CmnPoolBlockHeader)) {
		blockSize = sizeof(CmnPoolBlockHeader);
	}

	size_t blockCount = backingMemorySize / blockSize;
	if (prePreparedBlocks > blockCount) {
		prePreparedBlocks = blockCount;
	}

	// Won't ever happen. Who needs 2^31 allocations?
	if (blockCount > (1UL << 31)) {
		printf("Warning: the pool allocator only supports at most 2^31 concurrent allocations. Truncating backing memory.");
		backingMemorySize = (1ULL << 31) * blockSize;
	}

	if (prePreparedBlocks == 0) {
		prePreparedBlocks = 1;
	}

	for (size_t i = 0; i < prePreparedBlocks; i++) {
		size_t offset = i * blockSize;
		CmnPoolBlockHeader* header = (CmnPoolBlockHeader*)&backingMemory[offset];

		header->initialized = true;
		header->nextFreeBlock = i+1;
	}

	return CmnPool {
		/*backing=*/	backingMemory,
		/*blockCount=*/	blockCount,
		/*blockSize=*/	blockSize,
		/*firstFree=*/	(CmnPoolBlockHeader*)backingMemory,
	};
}

void* cmnPoolAllocRaw(CmnPool* pool, CmnResult* result) {
	if (pool->firstFree == nullptr) {
		CMN_SET_RESULT(result, CMN_OUT_OF_MEMORY);
		return nullptr;
	}

	CmnPoolBlockHeader* free = pool->firstFree;
	assert(free->initialized);

	CmnPoolBlockHeader* nextFree = cmnPoolHeaderAt(pool, free->nextFreeBlock);
	if (nextFree != nullptr && !nextFree->initialized) {
		nextFree->initialized	= true;
		nextFree->nextFreeBlock	= cmnPoolIndexOfBlockAt(pool, nextFree) + 1;
	}
	pool->firstFree = nextFree;

	memset(free, 0, pool->blockSize);

	CMN_SET_RESULT(result, CMN_SUCCESS);
	return (void*)free;
}

void cmnPoolFree(CmnPool* pool, void* address) {
	size_t blockIdx = cmnPoolIndexOfBlockAt(pool, address);
	if (blockIdx >= pool->blockCount) {
		return;
	}

	CmnPoolBlockHeader* header = cmnPoolHeaderAt(pool, blockIdx);
	header->initialized = true;
	header->nextFreeBlock = cmnPoolIndexOfBlockAt(pool, pool->firstFree);

	pool->firstFree = header;
}

