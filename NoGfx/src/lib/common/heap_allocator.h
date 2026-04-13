#ifndef CMN_HEAPALLOCATOR_H
#define CMN_HEAPALLOCATOR_H

#include <lib/common/common.h>
#include <lib/common/allocator.h>

void* cmnHeapAlloc(void*, size_t size, size_t align, CmnResult* result);
void* cmnHeapRealloc(void*, void* address, size_t oldSize, size_t newSize, size_t align, CmnResult* result);
void cmnHeapFree(void*, void* data, CmnResult* result);

CmnAllocator cmnHeapAllocator(void);

#endif // CMN_HEAPALLOCATOR_H

