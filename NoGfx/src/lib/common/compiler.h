#ifndef CMN_COMMONCOMPILERS_H
#define CMN_COMMONCOMPILERS_H

#ifdef __clang__
	// Active compiler is Clang.
	#define CMN_COMPILER_CLANG 1
#elif defined(__GNUC__)
	// Active compiler is GCC.
	#define CMN_COMPILER_GCC 1
#elif defined(_MSC_VER)
	// Active compiler is MSVC.
	#define CMN_COMPILER_MSVC 1
#else
	// Active compiler is unknown.
	#define CMN_COMPILER_UNKNOWN 1
#endif

#endif // CMN_COMMONCOMPILERS_H

