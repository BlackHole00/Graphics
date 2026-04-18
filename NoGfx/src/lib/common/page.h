#ifndef CMN_PAGE_H
#define CMN_PAGE_H

#include <lib/common/common.h>
#include <lib/common/arena.h>
#include <lib/common/pool.h>

// Permission flags for CmnPage.
typedef enum CmnPagePerm {
	CMN_PAGE_READABLE	= 0x1,
	CMN_PAGE_WRITABLE	= 0x2,
	CMN_PAGE_EXECUTABLE	= 0x4,
} CmnPagePerm;

// Bitset of CmnPagePerm.
typedef uint32_t CmnPagePerms;

// Cross-platform abstraction over memory-mapped pages.
typedef struct CmnPage {
	void* baseAddress;
	size_t size;
	CmnPagePerms permissions;
} CmnPage;

// Creates a page backed by OS virtual memory APIs.
//
// Inputs:
// - size: Page size in bytes.
// - permissions: Bitset of CmnPagePerm values.
// - result: Optional operation result.
//
// Results:
// - CMN_SUCCESS: Page creation succeeded.
// - CMN_VIRTUAL_MEMORY_ALLOCATION_FAILED: OS virtual memory allocation failed.
//
// Returns:
// - Created page on success, zero-initialized page on failure.
CmnPage cmnCreatePage(size_t size, CmnPagePerms permissions, CmnResult* result);

// Destroy a page and release its mapped memory.
void cmnDestroyPage(CmnPage page);

// Wrap page memory as an arena.
inline CmnArena cmnPageToArena(CmnPage page) {
	return cmnCreateArena((uint8_t*)page.baseAddress, page.size, false);
}

// Wrap page memory as a fixed-size pool.
inline CmnPool cmnPageToPool(CmnPage page, size_t blockSize) {
	return cmnCreatePool((uint8_t*)page.baseAddress, page.size, blockSize, 0);
}

#endif // CMN_PAGE_H

