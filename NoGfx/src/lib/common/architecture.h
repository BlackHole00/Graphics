#ifndef CMN_COMMONARCHITECTURE_H
#define CMN_COMMONARCHITECTURE_H

/**
	Defines for detecting the target CPU architecture and pointer width.
*/
#if defined(__aarch64__) || defined(_M_ARM64)
	/** The target architecture is ARM64. */
	#define CMN_ARCHITECTURE_ARM64 1
	/** The target architecture is 64 bit. */
	#define CMN_ARCHITECTURE_BITS 64
	/** Alias macro indicating a 64 bit target architecture. */
	#define CMN_ARCHITECTURE_64BITS 1

#elif defined(__x86_64__) || defined(_M_X64)
	/** The target architecture is AMD64. */
	#define CMN_ARCHITECTURE_AMD64 1
	/** The target architecture is 64 bit. */
	#define CMN_ARCHITECTURE_BITS 64
	/** Alias macro indicating a 64 bit target architecture. */
	#define CMN_ARCHITECTURE_64BITS 1

#elif defined(i386) || defined(__i386__) || defined(__i386) || defined(_M_IX86)
	/** The target architecture is i386. */
	#define CMN_ARCHITECTURE_I386 1
	/** The target architecture is 32 bit. */
	#define CMN_ARCHITECTURE_BITS 32
	/** Alias macro indicating a 32 bit target architecture. */
	#define CMN_ARCHITECTURE_32BITS 1

#else
	/** The target architecture is unknown. */
	#define CMN_ARCHITECTURE_UNKNOWN 1
#endif

#endif // CMN_COMMONARCHITECTURE_H

