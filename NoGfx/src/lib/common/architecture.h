#ifndef CMN_COMMONARCHITECTURE_H
#define CMN_COMMONARCHITECTURE_H

#if defined(__aarch64__) || defined(_M_ARM64)
	// Target architecture is ARM64.
	#define CMN_ARCHITECTURE_ARM64 1
	// Target architecture is 64-bit.
	#define CMN_ARCHITECTURE_BITS 64
	// Alias macro for 64-bit targets.
	#define CMN_ARCHITECTURE_64BITS 1

#elif defined(__x86_64__) || defined(_M_X64)
	// Target architecture is AMD64.
	#define CMN_ARCHITECTURE_AMD64 1
	// Target architecture is 64-bit.
	#define CMN_ARCHITECTURE_BITS 64
	// Alias macro for 64-bit targets.
	#define CMN_ARCHITECTURE_64BITS 1

#elif defined(i386) || defined(__i386__) || defined(__i386) || defined(_M_IX86)
	// Target architecture is i386.
	#define CMN_ARCHITECTURE_I386 1
	// Target architecture is 32-bit.
	#define CMN_ARCHITECTURE_BITS 32
	// Alias macro for 32-bit targets.
	#define CMN_ARCHITECTURE_32BITS 1

#else
	// Target architecture is unknown.
	#define CMN_ARCHITECTURE_UNKNOWN 1
#endif

#endif // CMN_COMMONARCHITECTURE_H

