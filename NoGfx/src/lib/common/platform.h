#ifndef CMN_COMMONPLATFORM_H
#define CMN_COMMONPLATFORM_H

#if defined(_WIN32) || defined(_WIN64)
	#define CMN_PLATFORM_WINDOWS 1
	#define CMN_SUBPLATFORM_WIN32 1

#elif defined(__CYGWIN__)
	#define CMN_PLATFORM_POSIX 1
	#define CMN_PLATFORM_WINDOWS 1

	#define CMN_SUBPLATFORM_CYGWIN 1
	#define CMN_SUBPLATFORM_WIN32 1

#elif defined(__linux__)
	#define CMN_PLATFORM_POSIX 1
	#define CMN_PLATFORM_LINUX 1

#elif defined(__APPLE__)
	#define CMN_PLATFORM_POSIX 1
	#define CMN_PLATFORM_DARWIN 1

#else
	#define CMN_PLATFORM_UNKNOWN 1
#endif

#endif // CMN_COMMONPLATFORM_H

