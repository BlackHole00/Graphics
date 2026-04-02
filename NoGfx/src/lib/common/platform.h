#ifndef GPU_COMMONPLATFORM_H
#define GPU_COMMONPLATFORM_H

#if defined(_WIN32) || defined(_WIN64)
	#define GPU_PLATFORM_WINDOWS 1
	#define GPU_SUBPLATFORM_WIN32 1

#elif defined(__CYGWIN__)
	#define GPU_PLATFORM_POSIX 1
	#define GPU_PLATFORM_WINDOWS 1

	#define GPU_SUBPLATFORM_CYGWIN 1
	#define GPU_SUBPLATFORM_WIN32 1

#elif defined(__linux__)
	#define GPU_PLATFORM_POSIX 1
	#define GPU_PLATFORM_LINUX 1

#elif defined(__APPLE__)
	#define GPU_PLATFORM_POSIX 1
	#define GPU_PLATFORM_DARWIN 1

#else
	#define GPU_PLATFORM_UNKNOWN 1
#endif

#endif // GPU_COMMONPLATFORM_H

