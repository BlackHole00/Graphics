#ifndef CMN_COMMONPLATFORM_H
#define CMN_COMMONPLATFORM_H

/**
	Defines for detecting the target operating system platform.
*/
#if defined(_WIN32) || defined(_WIN64)
	/** The target platform is Windows. */
	#define CMN_PLATFORM_WINDOWS 1
	/** The target subplatform uses the Win32 API. */
	#define CMN_SUBPLATFORM_WIN32 1

#elif defined(__CYGWIN__)
	/** The target platform supports POSIX APIs. */
	#define CMN_PLATFORM_POSIX 1
	/** The target platform is Windows. */
	#define CMN_PLATFORM_WINDOWS 1

	/** The target subplatform is Cygwin. */
	#define CMN_SUBPLATFORM_CYGWIN 1
	/** The target subplatform uses the Win32 API. */
	#define CMN_SUBPLATFORM_WIN32 1

#elif defined(__linux__)
	/** The target platform supports POSIX APIs. */
	#define CMN_PLATFORM_POSIX 1
	/** The target platform is Linux. */
	#define CMN_PLATFORM_LINUX 1

#elif defined(__APPLE__)
	/** The target platform supports POSIX APIs. */
	#define CMN_PLATFORM_POSIX 1
	/** The target platform is Darwin. */
	#define CMN_PLATFORM_DARWIN 1

#else
	/** The target platform is unknown. */
	#define CMN_PLATFORM_UNKNOWN 1
#endif

#endif // CMN_COMMONPLATFORM_H

