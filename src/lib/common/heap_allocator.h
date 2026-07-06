#ifndef CMN_HEAPALLOCATOR_H
#define CMN_HEAPALLOCATOR_H

#include <lib/common/common.h>
#include <lib/common/allocator.h>

// Allocates a raw block of memory from the process heap.
//
// Inputs:
// - size: Number of bytes to allocate.
// - align: Requested alignment in bytes.
// - result: Optional operation result.
//
// Results:
// - CMN_SUCCESS: Allocation succeeded.
// - CMN_OUT_OF_MEMORY: Heap allocator ran out of memory.
//
// Returns:
// - Allocated address, or nullptr on failure.
void* cmnHeapAllocRaw(size_t size, size_t align, CmnResult* result);

// Allocates count objects of type T from the process heap.
//
// Inputs:
// - count: Number of objects.
// - align: Requested alignment in bytes.
// - result: Optional operation result.
//
// Results:
// - CMN_SUCCESS: Allocation succeeded.
// - CMN_OUT_OF_MEMORY: Heap allocator ran out of memory.
//
// Returns:
// - Allocated object memory, or nullptr on failure.
template <typename T> T* cmnHeapAlloc(size_t count, size_t align, CmnResult* result) {
	return (T*)cmnHeapAllocRaw(count * sizeof(T), align, result);
}

// Allocates count objects of type T with default alignment.
//
// Inputs:
// - count: Number of objects.
// - result: Optional operation result.
//
// Results:
// - CMN_SUCCESS: Allocation succeeded.
// - CMN_OUT_OF_MEMORY: Heap allocator ran out of memory.
//
// Returns:
// - Allocated object memory, or nullptr on failure.
template <typename T> T* cmnHeapAlloc(size_t count, CmnResult* result) {
	return cmnHeapAlloc<T>(count, 0, result);
}

// Allocates one object of type T with default alignment.
//
// Inputs:
// - result: Optional operation result.
//
// Results:
// - CMN_SUCCESS: Allocation succeeded.
// - CMN_OUT_OF_MEMORY: Heap allocator ran out of memory.
//
// Returns:
// - Allocated object memory, or nullptr on failure.
template <typename T> T* cmnHeapAlloc(CmnResult* result) {
	return cmnHeapAlloc<T>(1, 0, result);
}

// Allocates a raw block of memory with default alignment.
//
// Inputs:
// - size: Number of bytes to allocate.
// - result: Optional operation result.
//
// Results:
// - CMN_SUCCESS: Allocation succeeded.
// - CMN_OUT_OF_MEMORY: Heap allocator ran out of memory.
//
// Returns:
// - Allocated address, or nullptr on failure.
inline void* cmnHeapAllocRaw(size_t size, CmnResult* result) {
	return cmnHeapAllocRaw(size, 0, result);
}

// Reallocates a raw memory block previously returned by cmnHeapAllocRaw.
//
// Inputs:
// - address: Previous allocation address.
// - oldSize: Previous allocation size in bytes.
// - newSize: New allocation size in bytes.
// - align: Requested alignment in bytes.
// - result: Optional operation result.
//
// Results:
// - CMN_SUCCESS: Reallocation succeeded.
// - CMN_OUT_OF_MEMORY: Heap allocator ran out of memory.
//
// Returns:
// - Reallocated address, or nullptr on failure.
void* cmnHeapReallocRaw(void* address, size_t oldSize, size_t newSize, size_t align, CmnResult* result);

// Reallocates count objects of type T on the process heap.
//
// Inputs:
// - address: Previous allocation address.
// - oldCount: Previous object count.
// - newCount: New object count.
// - align: Requested alignment in bytes.
// - result: Optional operation result.
//
// Results:
// - CMN_SUCCESS: Reallocation succeeded.
// - CMN_OUT_OF_MEMORY: Heap allocator ran out of memory.
//
// Returns:
// - Reallocated object memory, or nullptr on failure.
template <typename T> T* cmnHeapRealloc(T* address, size_t oldCount, size_t newCount, size_t align, CmnResult* result) {
	return (T*)cmnHeapReallocRaw(address, oldCount * sizeof(T), newCount * sizeof(T), align, result);
}

// Reallocates count objects of type T with default alignment.
//
// Inputs:
// - address: Previous allocation address.
// - oldCount: Previous object count.
// - newCount: New object count.
// - result: Optional operation result.
//
// Results:
// - CMN_SUCCESS: Reallocation succeeded.
// - CMN_OUT_OF_MEMORY: Heap allocator ran out of memory.
//
// Returns:
// - Reallocated object memory, or nullptr on failure.
template <typename T> T* cmnHeapRealloc(T* address, size_t oldCount, size_t newCount, CmnResult* result) {
	return cmnHeapRealloc<T>(address, oldCount, newCount, 0, result);
}

// Reallocates a raw memory block with default alignment.
//
// Inputs:
// - address: Previous allocation address.
// - oldSize: Previous allocation size in bytes.
// - newSize: New allocation size in bytes.
// - result: Optional operation result.
//
// Results:
// - CMN_SUCCESS: Reallocation succeeded.
// - CMN_OUT_OF_MEMORY: Heap allocator ran out of memory.
//
// Returns:
// - Reallocated address, or nullptr on failure.
inline void* cmnHeapReallocRaw(void* address, size_t oldSize, size_t newSize, CmnResult* result) {
	return cmnHeapReallocRaw(address, oldSize, newSize, 0, result);
}

// Releases memory allocated from the process heap.
//
// Inputs:
// - data: Address to free.
void cmnHeapFree(void* data);

// Returns a CmnAllocator backed by the process heap.
//
// Returns:
// - Allocator backed by the process heap.
CmnAllocator cmnHeapAllocator(void);

#endif // CMN_HEAPALLOCATOR_H

