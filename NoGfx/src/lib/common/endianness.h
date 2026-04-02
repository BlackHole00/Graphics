#ifndef GPU_COMMONENDIANNESS_H
#define GPU_COMMONENDIANNESS_H

#include "architecture.h"
#include "platform.h"

#if defined(GPU_ARCHITECTURE_AMD64) || defined(GPU_ARCHITECTURE_I386)
	#define GPU_ENDIANNESS_LITTLE 1

#elif defined(GPU_ARCHITECTURE_ARM64)
	#if defined(GPU_PLATFORM_LINUX) || defined(GPU_PLATFORM_DARWIN) || defined(GPU_PLATFORM_ANDROID)
		#define GPU_ENDIANNESS_LITTLE 1
	#endif

#else
	#define GPU_ENDIANNESS_UNKNOWN 1

#endif

#endif // GPU_COMMONENDIANNESS_H
