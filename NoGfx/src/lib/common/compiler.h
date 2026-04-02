#ifndef GPU_COMMONCOMPILERS_H
#define GPU_COMMONCOMPILERS_H

#ifdef __clang__
	#define STD_COMPILER_CLANG 1
#elif defined(__GNUC__)
	#define STD_COMPILER_GCC 1
#elif defined(_MSC_VER)
	#define STD_COMPILER_MSVC 1
#else
	#define STD_COMPILER_UNKNOWN 1
#endif

#endif // GPU_COMMONCOMPILERS_H

