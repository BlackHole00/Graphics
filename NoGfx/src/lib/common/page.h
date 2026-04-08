#ifndef CMN_PAGE_H
#define CMN_PAGE_H

#include <lib/common/common.h>
#include <lib/common/arena.h>
#include <lib/common/pool.h>

typedef enum CmnPagePerm {
	CMN_PAGE_READABLE	= 0x1,
	CMN_PAGE_WRITABLE	= 0x2,
	CMN_PAGE_EXECUTABLE	= 0x4,
} CmnPagePerm;
typedef uint32_t CmnPagePerms;

typedef struct CmnPage {
	void* baseAddress;
	size_t size;
	CmnPagePerms permissions;
} CmnPage;

// NOTE: Platform specific
CmnPage cmnCreatePage(size_t size, CmnPagePerms permissions, CmnResult* result);
// NOTE: Platform specific
void cmnDestroyPage(CmnPage page);

inline CmnArena cmnPageToArena(CmnPage page) {
	return cmnCreateArena((uint8_t*)page.baseAddress, page.size, false);
}
inline CmnPool cmnPageToPool(CmnPage page, size_t blockSize) {
	return cmnCreatePool((uint8_t*)page.baseAddress, page.size, blockSize, 0);
}

#endif // CMN_PAGE_H

