#ifndef CMN_COMMONENDIANNESS_H
#define CMN_COMMONENDIANNESS_H

#include "architecture.h"
#include "platform.h"

#if defined(CMN_ARCHITECTURE_AMD64) || defined(CMN_ARCHITECTURE_I386)
	#define CMN_ENDIANNESS_LITTLE 1

#elif defined(CMN_ARCHITECTURE_ARM64)
	#if defined(CMN_PLATFORM_LINUX) || defined(CMN_PLATFORM_DARWIN) || defined(CMN_PLATFORM_ANDROID)
		#define CMN_ENDIANNESS_LITTLE 1
	#endif

#else
	#define CMN_ENDIANNESS_UNKNOWN 1

#endif

#endif // CMN_COMMONENDIANNESS_H
