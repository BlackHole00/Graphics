#ifndef CMN_COMMONCOMPILERS_H
#define CMN_COMMONCOMPILERS_H

#ifdef __clang__
	#define CMN_COMPILER_CLANG 1
#elif defined(__GNUC__)
	#define CMN_COMPILER_GCC 1
#elif defined(_MSC_VER)
	#define CMN_COMPILER_MSVC 1
#else
	#define CMN_COMPILER_UNKNOWN 1
#endif

#endif // CMN_COMMONCOMPILERS_H

