#ifndef CMN_COMMONCOMPILERS_H
#define CMN_COMMONCOMPILERS_H

/**
	Defines for detecting the active compiler.
*/
#ifdef __clang__
	/** The active compiler is Clang. */
	#define CMN_COMPILER_CLANG 1
#elif defined(__GNUC__)
	/** The active compiler is GCC. */
	#define CMN_COMPILER_GCC 1
#elif defined(_MSC_VER)
	/** The active compiler is MSVC. */
	#define CMN_COMPILER_MSVC 1
#else
	/** The active compiler is unknown. */
	#define CMN_COMPILER_UNKNOWN 1
#endif

#endif // CMN_COMMONCOMPILERS_H

