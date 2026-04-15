#ifndef CMN_HEAPALLOCATOR_H
#define CMN_HEAPALLOCATOR_H

#include <lib/common/common.h>
#include <lib/common/allocator.h>

/**
	Heap-backed allocation function compatible with CmnAllocatorVTable.
	The allocator state pointer parameter is unused.

	@param size Size in bytes to allocate.
	@param align Requested alignment.
	@param[out] result The result of the operation.

	@return Allocated address, or `nullptr` on failure.
	@retval CMN_SUCCESS Allocation completed successfully.
	@retval CMN_OUT_OF_MEMORY Heap allocation failed.
*/
void* cmnHeapAlloc(size_t size, size_t align, CmnResult* result);

/**
	Heap-backed reallocation function compatible with CmnAllocatorVTable.
	The allocator state pointer parameter is unused.

	@param address Previous allocation address.
	@param oldSize Previous allocation size in bytes.
	@param newSize New allocation size in bytes.
	@param align Requested alignment.
	@param[out] result The result of the operation.

	@return Reallocated address, or `nullptr` on failure.
	@retval CMN_SUCCESS Reallocation completed successfully.
	@retval CMN_OUT_OF_MEMORY Heap reallocation failed.
*/
void* cmnHeapRealloc(void* address, size_t oldSize, size_t newSize, size_t align, CmnResult* result);

/**
	Heap-backed free function compatible with CmnAllocatorVTable.
	The allocator state pointer parameter is unused.

	@param data Address to free.
	@param[out] result The result of the operation.
	@retval CMN_SUCCESS Memory was freed successfully.
*/
void cmnHeapFree(void* data, CmnResult* result);

/**
	Creates a CmnAllocator that uses the process heap.

	@return Heap-backed allocator.
*/
CmnAllocator cmnHeapAllocator(void);

#endif // CMN_HEAPALLOCATOR_H

