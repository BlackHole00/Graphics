#ifndef CMN_COMMONPLATFORM_H
#define CMN_COMMONPLATFORM_H

#if defined(_WIN32) || defined(_WIN64)
	// Target platform is Windows.
	#define CMN_PLATFORM_WINDOWS 1
	// Subplatform uses the Win32 API.
	#define CMN_SUBPLATFORM_WIN32 1

#elif defined(__CYGWIN__)
	// Target platform supports POSIX APIs.
	#define CMN_PLATFORM_POSIX 1
	// Target platform is Windows.
	#define CMN_PLATFORM_WINDOWS 1

	// Target subplatform is Cygwin.
	#define CMN_SUBPLATFORM_CYGWIN 1
	// Target subplatform uses the Win32 API.
	#define CMN_SUBPLATFORM_WIN32 1

#elif defined(__linux__)
	// Target platform supports POSIX APIs.
	#define CMN_PLATFORM_POSIX 1
	// Target platform is Linux.
	#define CMN_PLATFORM_LINUX 1

#elif defined(__APPLE__)
	// Target platform supports POSIX APIs.
	#define CMN_PLATFORM_POSIX 1
	// Target platform is Darwin.
	#define CMN_PLATFORM_DARWIN 1

#else
	// Target platform is unknown.
	#define CMN_PLATFORM_UNKNOWN 1
#endif

#endif // CMN_COMMONPLATFORM_H

