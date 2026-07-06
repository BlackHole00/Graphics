#include <lib/common/common.h>

#if !defined(CMN_LANGUAGE_CPP) && CMN_LANGUAGE_VERSION < 2011
	#error NoGfx requires a cpp compiler supporting C++ 11.
#endif


#ifdef CMN_PLATFORM_DARWIN
	#include "build_darwin.mm"
#else
	#error NoGfx currently supports only macOs.
#endif

