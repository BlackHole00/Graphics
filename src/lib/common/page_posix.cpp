#include "page.h"

#ifdef CMN_PLATFORM_POSIX

#include <sys/mman.h>

static int cmnPermissionsToPosix(CmnPagePerms permissions) {
	int unixPerms = 0;

	if (permissions & CMN_PAGE_READABLE) {
		unixPerms |= PROT_READ;
	}
	if (permissions & CMN_PAGE_WRITABLE) {
		unixPerms |= PROT_WRITE;
	}
	if (permissions & CMN_PAGE_EXECUTABLE) {
		unixPerms |= PROT_EXEC;
	}

	return unixPerms;
}

CmnPage cmnCreatePage(size_t size, CmnPagePerms permissions, CmnResult* result) {
	int unixPerms = cmnPermissionsToPosix(permissions);

	void* baseAddress = mmap(NULL, size, unixPerms, MAP_ANON | MAP_PRIVATE, -1, 0);
	if (baseAddress == MAP_FAILED) {
		CMN_SET_RESULT(result, CMN_VIRTUAL_MEMORY_ALLOCATION_FAILED);
		return {};
	}

	CMN_SET_RESULT(result, CMN_SUCCESS);
	return CmnPage {
		/*baseAddress=*/	baseAddress,
		/*size=*/		size,
		/*permissions=*/	permissions,
	};
}

void cmnDestroyPage(CmnPage page) {
	if (page.baseAddress == nullptr) {
		return;
	}

	munmap(page.baseAddress, page.size);

	page = {};
}

#endif

