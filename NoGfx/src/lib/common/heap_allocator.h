#ifndef CMN_HEAPALLOCATOR_H
#define CMN_HEAPALLOCATOR_H

#include <lib/common/common.h>
#include <lib/common/allocator.h>

// Allocates memory from the process heap.
//
// Inputs:
// - size: Number of bytes to allocate.
// - align: Requested alignment.
// - result: Optional operation result.
//
// Results:
// - CMN_SUCCESS: Allocation succeeded.
// - CMN_OUT_OF_MEMORY: Heap allocator ran out of memory.
//
// Returns:
// - Allocated address, or nullptr on failure.
void* cmnHeapAlloc(size_t size, size_t align, CmnResult* result);

inline void* cmnHeapAlloc(size_t size, CmnResult* result) {
	return cmnHeapAlloc(size, 0, result);
}

// Reallocates memory previously returned by cmnHeapAlloc.
//
// Inputs:
// - address: Previous allocation address.
// - oldSize: Previous allocation size in bytes.
// - newSize: New allocation size in bytes.
// - align: Requested alignment.
// - result: Optional operation result.
//
// Results:
// - CMN_SUCCESS: Reallocation succeeded.
// - CMN_OUT_OF_MEMORY: Heap allocator ran out of memory.
//
// Returns:
// - Reallocated address, or nullptr on failure.
void* cmnHeapRealloc(void* address, size_t oldSize, size_t newSize, size_t align, CmnResult* result);

// Releases memory allocated from the process heap.
//
// Inputs:
// - data: Address to free.
void cmnHeapFree(void* data);

// Returns a CmnAllocator backed by the process heap.
CmnAllocator cmnHeapAllocator(void);

#endif // CMN_HEAPALLOCATOR_H

