#ifndef CMN_PAGE_H
#define CMN_PAGE_H

#include <lib/common/common.h>
#include <lib/common/arena.h>
#include <lib/common/pool.h>

/**
	A permission that can be applied to a CmnPage.
	@see CmnPagePerms
*/
typedef enum CmnPagePerm {
	/** The page can be read. */
	CMN_PAGE_READABLE	= 0x1,
	/** The page can be written to. */
	CMN_PAGE_WRITABLE	= 0x2,
	/** The page can be executed. */
	CMN_PAGE_EXECUTABLE	= 0x4,
} CmnPagePerm;

/**
	Bitset of representing all the possible combinations of page permissions.
	@see CmnPagePerm
*/
typedef uint32_t CmnPagePerms;

/**
	A cross platform abstraction for handling mmapped memory pages.
*/
typedef struct CmnPage {
	/** The base address of the mmapped memory. */
	void* baseAddress;
	/** The size in bytes of the mmapped memory. */
	size_t size;
	/** The permissions applied to the page. */
	CmnPagePerms permissions;
} CmnPage;

/**
	Creates a new CmnPage, allocating mmapped memory.
	The related functionality maps directly to `mmap` on posix systems and `VirtualAlloc` on Windows. The allocated
	memory is always reserved and never committed.

	@param size The size of the page.
	@param permissions The permissions of the page. Bitset of CmnPagePerm flags.
	@param[out] result The result of the operation. If `nullptr` the result will be discarded.

	@return The created page if no error occurred. A zero-initialized page otherwise.
	@retval CMN_SUCCESS Page created successfully
	@retval CMN_VIRTUAL_MEMORY_ALLOCATION_FAILED Page creation failed

	@remark Platform specific.
	@relates CmnPage
*/
CmnPage cmnCreatePage(size_t size, CmnPagePerms permissions, CmnResult* result);

/**
	Destroys a CmnPage and frees the related mmapped memory.
	The related functionality maps directly to `munmap` on posix sistems and `VirtualFree` on Windows.

	@remark Platform specific.
	@relates CmnPage
*/
void cmnDestroyPage(CmnPage page);

/**
	Creates a new CmnArena using the page's memory.

	@relates CmnPage
	@see CmnArena
*/
inline CmnArena cmnPageToArena(CmnPage page) {
	return cmnCreateArena((uint8_t*)page.baseAddress, page.size, false);
}

/**
	Creates a new CmnPage using the page's memory.

	@relates CmnPage
	@see CmnPool
*/
inline CmnPool cmnPageToPool(CmnPage page, size_t blockSize) {
	return cmnCreatePool((uint8_t*)page.baseAddress, page.size, blockSize, 0);
}

#endif // CMN_PAGE_H

