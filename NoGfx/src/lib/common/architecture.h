#ifndef GPU_COMMONARCHITECTURE_H
#define GPU_COMMONARCHITECTURE_H

#if defined(__aarch64__) || defined(_M_ARM64)
	#define GPU_ARCHITECTURE_ARM64 1
	#define GPU_ARCHITECTURE_BITS 64
	#define GPU_ARCHITECTURE_64BITS 1

#elif defined(__x86_64__) || defined(_M_X64)
	#define GPU_ARCHITECTURE_AMD64 1
	#define GPU_ARCHITECTURE_BITS 64
	#define GPU_ARCHITECTURE_64BITS 1

#elif defined(i386) || defined(__i386__) || defined(__i386) || defined(_M_IX86)
	#define GPU_ARCHITECTURE_I386 1
	#define GPU_ARCHITECTURE_BITS 32
	#define GPU_ARCHITECTURE_32BITS 1

#else
	#define GPU_ARCHITECTURE_UNKNOWN 1
#endif

#endif // GPU_COMMONARCHITECTURE_H

