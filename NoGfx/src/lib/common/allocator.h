#ifndef CMN_ALLOCATOR_H
#define CMN_ALLOCATOR_H

#include <lib/common/common.h>

/**
	Virtual table defining allocator operations.
	@see CmnAllocator
*/
typedef struct CmnAllocatorVTable {
	/** Allocates a memory block. */
	void*	(*alloc		)(void* data, size_t size, size_t align, CmnResult* result);
	/** Reallocates a memory block. */
	void*	(*realloc	)(void* data, void* address, size_t oldSize, size_t newSize, size_t align, CmnResult* result);
	/** Frees a memory block. */
	void	(*free		)(void* data, void* address, CmnResult* result);
	/** Frees all memory managed by the allocator when supported. */
	void	(*freeAll	)(void* data, CmnResult* result);
} CmnAllocatorVTable;

/**
	Type-erased allocator handle.
*/
typedef struct CmnAllocator {
	/** The allocator operation table. */
	const CmnAllocatorVTable* vtable;
	/** Opaque allocator state passed to the virtual functions. */
	void* data;
} CmnAllocator;

/**
	Allocates an array of elements of type `T`.

	@param allocator The allocator to use.
	@param count The number of elements to allocate.
	@param align The requested memory alignment.
	@param[out] result The result of the operation.

	@return A pointer to the allocated memory, or `nullptr` on failure.
	@retval CMN_SUCCESS Allocation completed successfully.
	@retval CMN_OUT_OF_MEMORY The underlying allocator could not allocate memory.
	@retval CMN_UNSUPPORTED_OPERATION The underlying allocator does not support allocation.
	@relates CmnAllocator
*/
template <typename T> T* cmnAlloc(CmnAllocator allocator, size_t count, size_t align, CmnResult* result) {
	return (T*)allocator.vtable->alloc(allocator.data, count * sizeof(T), align, result);
}

/**
	Allocates an array of elements of type `T` with default alignment.

	@param allocator The allocator to use.
	@param count The number of elements to allocate.
	@param[out] result The result of the operation.

	@return A pointer to the allocated memory, or `nullptr` on failure.
	@retval CMN_SUCCESS Allocation completed successfully.
	@retval CMN_OUT_OF_MEMORY The underlying allocator could not allocate memory.
	@retval CMN_UNSUPPORTED_OPERATION The underlying allocator does not support allocation.
	@relates CmnAllocator
*/
template <typename T> T* cmnAlloc(CmnAllocator allocator, size_t count, CmnResult* result) {
	return cmnAlloc<T>(allocator, count, 0, result);
}

/**
	Allocates one element of type `T` with default alignment.

	@param allocator The allocator to use.
	@param[out] result The result of the operation.

	@return A pointer to the allocated memory, or `nullptr` on failure.
	@retval CMN_SUCCESS Allocation completed successfully.
	@retval CMN_OUT_OF_MEMORY The underlying allocator could not allocate memory.
	@retval CMN_UNSUPPORTED_OPERATION The underlying allocator does not support allocation.
	@relates CmnAllocator
*/
template <typename T> T* cmnAlloc(CmnAllocator allocator, CmnResult* result) {
	return cmnAlloc<T>(allocator, 1, 0, result);
}

/**
	Allocates a raw block of bytes.

	@param allocator The allocator to use.
	@param size The number of bytes to allocate.
	@param align The requested memory alignment.
	@param[out] result The result of the operation.

	@return A pointer to the allocated memory, or `nullptr` on failure.
	@retval CMN_SUCCESS Allocation completed successfully.
	@retval CMN_OUT_OF_MEMORY The underlying allocator could not allocate memory.
	@retval CMN_UNSUPPORTED_OPERATION The underlying allocator does not support allocation.
	@relates CmnAllocator
*/
template <>
inline void* cmnAlloc(CmnAllocator allocator, size_t size, size_t align, CmnResult* result) {
	return allocator.vtable->alloc(allocator.data, size, align, result);
}

/**
	Reallocates an array of elements of type `T`.

	@param allocator The allocator to use.
	@param address The previous allocation address.
	@param oldCount The previous element count.
	@param newCount The new element count.
	@param align The requested memory alignment.
	@param[out] result The result of the operation.

	@return A pointer to the reallocated memory, or `nullptr` on failure.
	@retval CMN_SUCCESS Reallocation completed successfully.
	@retval CMN_OUT_OF_MEMORY The underlying allocator could not reallocate memory.
	@retval CMN_UNSUPPORTED_OPERATION The underlying allocator does not support reallocation.
	@relates CmnAllocator
*/
template <typename T> T* cmnRealloc(CmnAllocator allocator, void* address, size_t oldCount, size_t newCount, size_t align, CmnResult* result) {
	return (T*)allocator.vtable->realloc(allocator.data, address, newCount * sizeof(T), oldCount * sizeof(T), align, result);
}

/**
	Reallocates an array of elements of type `T` with default alignment.

	@param allocator The allocator to use.
	@param address The previous allocation address.
	@param oldCount The previous element count.
	@param newCount The new element count.
	@param[out] result The result of the operation.

	@return A pointer to the reallocated memory, or `nullptr` on failure.
	@retval CMN_SUCCESS Reallocation completed successfully.
	@retval CMN_OUT_OF_MEMORY The underlying allocator could not reallocate memory.
	@retval CMN_UNSUPPORTED_OPERATION The underlying allocator does not support reallocation.
	@relates CmnAllocator
*/
template <typename T> T* cmnRealloc(CmnAllocator allocator, void* address, size_t oldCount, size_t newCount, CmnResult* result) {
	return cmnRealloc<T>(allocator, address, oldCount, newCount, 0, result);
}

/**
	Reallocates a raw block of bytes.

	@param allocator The allocator to use.
	@param address The previous allocation address.
	@param newSize The new size in bytes.
	@param oldSize The previous size in bytes.
	@param align The requested memory alignment.
	@param[out] result The result of the operation.

	@return A pointer to the reallocated memory, or `nullptr` on failure.
	@retval CMN_SUCCESS Reallocation completed successfully.
	@retval CMN_OUT_OF_MEMORY The underlying allocator could not reallocate memory.
	@retval CMN_UNSUPPORTED_OPERATION The underlying allocator does not support reallocation.
	@relates CmnAllocator
*/
template <>
inline void* cmnRealloc(CmnAllocator allocator, void* address, size_t newSize, size_t oldSize, size_t align, CmnResult* result) {
	return allocator.vtable->realloc(allocator.data, address, oldSize, newSize, align, result);
}

/**
	Frees a previously allocated memory block.

	@param allocator The allocator to use.
	@param address The address to free.
	@param[out] result The result of the operation.
	@retval CMN_SUCCESS Free operation completed successfully.
	@retval CMN_UNSUPPORTED_OPERATION The underlying allocator does not support freeing a single allocation.
	@relates CmnAllocator
*/
inline void cmnFree(CmnAllocator allocator, void* address, CmnResult* result = nullptr) {
	allocator.vtable->free(allocator.data, address, result);
}

/**
	Frees all memory managed by the allocator when supported.

	@param allocator The allocator to use.
	@param[out] result The result of the operation.
	@retval CMN_SUCCESS Free-all operation completed successfully.
	@retval CMN_UNSUPPORTED_OPERATION The underlying allocator does not support free-all.
	@relates CmnAllocator
*/
inline void cmnFreeAll(CmnAllocator allocator, CmnResult* result = nullptr) {
	allocator.vtable->freeAll(allocator.data, result);
}

#endif // CMN_ALLOCATOR_H

