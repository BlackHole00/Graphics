#ifndef CMN_ALLOCATOR_H
#define CMN_ALLOCATOR_H

#include <lib/common/common.h>

typedef struct CmnAllocatorVTable {
	void*	(*alloc		)(void* data, size_t size, size_t align, CmnResult* result);
	void*	(*realloc	)(void* data, void* address, size_t oldSize, size_t newSize, size_t align, CmnResult* result);
	void	(*free		)(void* data, void* address, CmnResult* result);
	void	(*freeAll	)(void* data, CmnResult* result);
} CmnAllocatorVTable;

typedef struct CmnAllocator {
	const CmnAllocatorVTable* vtable;
	void* data;
} CmnAllocator;

template <typename T> T* cmnAlloc(CmnAllocator allocator, size_t count, size_t align, CmnResult* result) {
	return (T*)allocator.vtable->alloc(allocator.data, count * sizeof(T), align, result);
}
template <typename T> T* cmnAlloc(CmnAllocator allocator, size_t count, CmnResult* result) {
	return cmnAlloc<T>(allocator, count, 0, result);
}
template <typename T> T* cmnAlloc(CmnAllocator allocator, CmnResult* result) {
	return cmnAlloc<T>(allocator, 1, 0, result);
}
template <>
inline void* cmnAlloc(CmnAllocator allocator, size_t size, size_t align, CmnResult* result) {
	return allocator.vtable->alloc(allocator.data, size, align, result);
}

template <typename T> T* cmnRealloc(CmnAllocator allocator, void* address, size_t oldCount, size_t newCount, size_t align, CmnResult* result) {
	return (T*)allocator.vtable->realloc(allocator.data, address, newCount * sizeof(T), oldCount * sizeof(T), align, result);
}
template <typename T> T* cmnRealloc(CmnAllocator allocator, void* address, size_t oldCount, size_t newCount, CmnResult* result) {
	return cmnRealloc<T>(allocator, address, oldCount, newCount, 0, result);
}
template <>
inline void* cmnRealloc(CmnAllocator allocator, void* address, size_t newSize, size_t oldSize, size_t align, CmnResult* result) {
	return allocator.vtable->realloc(allocator.data, address, oldSize, newSize, align, result);
}

inline void cmnFree(CmnAllocator allocator, void* address, CmnResult* result = nullptr) {
	allocator.vtable->free(allocator.data, address, result);
}

inline void cmnFreeAll(CmnAllocator allocator, CmnResult* result = nullptr) {
	allocator.vtable->freeAll(allocator.data, result);
}

#endif // CMN_ALLOCATOR_H

