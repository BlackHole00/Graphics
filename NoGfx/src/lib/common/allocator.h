#ifndef CMN_ALLOCATOR_H
#define CMN_ALLOCATOR_H

#include <lib/common/common.h>

// Virtual table defining allocator operations.
typedef struct CmnAllocatorVTable {
	void*	(*alloc		)(void* data, size_t size, size_t align, CmnResult* result);
	void*	(*realloc	)(void* data, void* address, size_t oldSize, size_t newSize, size_t align, CmnResult* result);
	void	(*free		)(void* data, void* address, CmnResult* result);
	void	(*freeAll	)(void* data, CmnResult* result);
} CmnAllocatorVTable;

// Allocator handle.
typedef struct CmnAllocator {
	const CmnAllocatorVTable* vtable;
	void* data;
} CmnAllocator;

// cmnAlloc allocates count elements of type T.
//
// Inputs:
// - allocator: Allocator to use.
// - count: Number of elements.
// - align: Requested alignment in bytes.
// - result: Optional operation result.
//
// Results:
// - CMN_SUCCESS: Allocation succeeded.
// - CMN_OUT_OF_MEMORY: Allocator ran out of memory.
// - CMN_UNSUPPORTED_OPERATION: Allocator does not support allocation.
//
// Returns:
// - Pointer to allocated memory, or nullptr on failure.
template <typename T> T* cmnAlloc(CmnAllocator allocator, size_t count, size_t align, CmnResult* result) {
	return (T*)allocator.vtable->alloc(allocator.data, count * sizeof(T), align, result);
}

// cmnAlloc allocates count elements of type T with default alignment.
//
// Inputs:
// - allocator: Allocator to use.
// - count: Number of elements.
// - result: Optional operation result.
//
// Results:
// - CMN_SUCCESS: Allocation succeeded.
// - CMN_OUT_OF_MEMORY: Allocator ran out of memory.
// - CMN_UNSUPPORTED_OPERATION: Allocator does not support allocation.
//
// Returns:
// - Pointer to allocated memory, or nullptr on failure.
template <typename T> T* cmnAlloc(CmnAllocator allocator, size_t count, CmnResult* result) {
	return cmnAlloc<T>(allocator, count, 0, result);
}

// cmnAlloc allocates one element of type T with default alignment.
//
// Inputs:
// - allocator: Allocator to use.
// - result: Optional operation result.
//
// Results:
// - CMN_SUCCESS: Allocation succeeded.
// - CMN_OUT_OF_MEMORY: Allocator ran out of memory.
// - CMN_UNSUPPORTED_OPERATION: Allocator does not support allocation.
//
// Returns:
// - Pointer to allocated memory, or nullptr on failure.
template <typename T> T* cmnAlloc(CmnAllocator allocator, CmnResult* result) {
	return cmnAlloc<T>(allocator, 1, 0, result);
}

// cmnAlloc allocates a raw block of bytes.
//
// Inputs:
// - allocator: Allocator to use.
// - size: Number of bytes.
// - align: Requested alignment in bytes.
// - result: Optional operation result.
//
// Results:
// - CMN_SUCCESS: Allocation succeeded.
// - CMN_OUT_OF_MEMORY: Allocator ran out of memory.
// - CMN_UNSUPPORTED_OPERATION: Allocator does not support allocation.
//
// Returns:
// - Pointer to allocated memory, or nullptr on failure.
template <>
inline void* cmnAlloc(CmnAllocator allocator, size_t size, size_t align, CmnResult* result) {
	return allocator.vtable->alloc(allocator.data, size, align, result);
}

// cmnRealloc reallocates an array of type T.
//
// Inputs:
// - allocator: Allocator to use.
// - address: Existing allocation.
// - oldCount: Previous element count.
// - newCount: New element count.
// - align: Requested alignment in bytes.
// - result: Optional operation result.
//
// Results:
// - CMN_SUCCESS: Reallocation succeeded.
// - CMN_OUT_OF_MEMORY: Allocator ran out of memory.
// - CMN_UNSUPPORTED_OPERATION: Allocator does not support reallocation.
//
// Returns:
// - Pointer to reallocated memory, or nullptr on failure.
template <typename T> T* cmnRealloc(CmnAllocator allocator, void* address, size_t oldCount, size_t newCount, size_t align, CmnResult* result) {
	return (T*)allocator.vtable->realloc(allocator.data, address, newCount * sizeof(T), oldCount * sizeof(T), align, result);
}

// cmnRealloc reallocates an array of type T with default alignment.
//
// Inputs:
// - allocator: Allocator to use.
// - address: Existing allocation.
// - oldCount: Previous element count.
// - newCount: New element count.
// - result: Optional operation result.
//
// Results:
// - CMN_SUCCESS: Reallocation succeeded.
// - CMN_OUT_OF_MEMORY: Allocator ran out of memory.
// - CMN_UNSUPPORTED_OPERATION: Allocator does not support reallocation.
//
// Returns:
// - Pointer to reallocated memory, or nullptr on failure.
template <typename T> T* cmnRealloc(CmnAllocator allocator, void* address, size_t oldCount, size_t newCount, CmnResult* result) {
	return cmnRealloc<T>(allocator, address, oldCount, newCount, 0, result);
}

// cmnRealloc reallocates a raw block of bytes.
//
// Inputs:
// - allocator: Allocator to use.
// - address: Existing allocation.
// - newSize: New size in bytes.
// - oldSize: Previous size in bytes.
// - align: Requested alignment in bytes.
// - result: Optional operation result.
//
// Results:
// - CMN_SUCCESS: Reallocation succeeded.
// - CMN_OUT_OF_MEMORY: Allocator ran out of memory.
// - CMN_UNSUPPORTED_OPERATION: Allocator does not support reallocation.
//
// Returns:
// - Pointer to reallocated memory, or nullptr on failure.
template <>
inline void* cmnRealloc(CmnAllocator allocator, void* address, size_t newSize, size_t oldSize, size_t align, CmnResult* result) {
	return allocator.vtable->realloc(allocator.data, address, oldSize, newSize, align, result);
}

// cmnFree releases a previously allocated memory block.
//
// Inputs:
// - allocator: Allocator to use.
// - address: Allocation address.
// - result: Optional operation result.
//
// Results:
// - CMN_SUCCESS: Free succeeded.
// - CMN_UNSUPPORTED_OPERATION: Allocator does not support free.
inline void cmnFree(CmnAllocator allocator, void* address, CmnResult* result = nullptr) {
	allocator.vtable->free(allocator.data, address, result);
}

// cmnFreeAll releases all memory managed by the allocator, when supported.
//
// Inputs:
// - allocator: Allocator to use.
// - result: Optional operation result.
//
// Results:
// - CMN_SUCCESS: Free-all succeeded.
// - CMN_UNSUPPORTED_OPERATION: Allocator does not support free-all.
inline void cmnFreeAll(CmnAllocator allocator, CmnResult* result = nullptr) {
	allocator.vtable->freeAll(allocator.data, result);
}

#endif // CMN_ALLOCATOR_H

