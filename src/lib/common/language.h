#ifndef CMN_COMMONLANGUAGE_H
#define CMN_COMMONLANGUAGE_H

#ifdef __cplusplus
	// Active source language is C++.
	#define CMN_LANGUAGE_CPP 1

	#if __cplusplus == 1
		#define CMN_LANGUAGE_CPP_OLD 1
		#define CMN_LANGUAGE_VERSION 0
	#elif __cplusplus == 199711L
		#define CMN_LANGUAGE_CPP_98 1
		#define CMN_LANGUAGE_VERSION 1998
	#elif __cplusplus == 201103L
		#define CMN_LANGUAGE_CPP_11 1
		#define CMN_LANGUAGE_VERSION 2011
	#elif __cplusplus == 201402L
		#define CMN_LANGUAGE_CPP_14 1
		#define CMN_LANGUAGE_VERSION 2014
	#elif __cplusplus == 201703L
		#define CMN_LANGUAGE_CPP_17 1
		#define CMN_LANGUAGE_VERSION 2017
	#elif __cplusplus == 202002L
		#define CMN_LANGUAGE_CPP_20 1
		#define CMN_LANGUAGE_VERSION 2020
	#elif __cplusplus == 202302L
		#define CMN_LANGUAGE_CPP_23 1
		#define CMN_LANGUAGE_VERSION 2023
	#endif
#else
	// Active source language is C.
	#define CMN_LANGUAGE_C 1

	#ifndef CMN_COMPILER_MSVC
		#ifndef __STDC__ 
			#define CMN_LANGUAGE_C_OLD 1
			#define CMN_LANGUAGE_VERSION 0
		#elif !defined(__STDC_VERSION__)
			#define CMN_LANGUAGE_C_89 1
			#define CMN_LANGUAGE_VERSION 1989
		#elif __STDC_VERSION__ == 199409L
			#define CMN_LANGUAGE_C_95 1
			#define CMN_LANGUAGE_VERSION 1995
		#elif __STDC_VERSION__ == 199901L
			#define CMN_LANGUAGE_C_99 1
			#define CMN_LANGUAGE_VERSION 1999
		#elif __STDC_VERSION__ == 201112L
			#define CMN_LANGUAGE_C_11 1
			#define CMN_LANGUAGE_VERSION 2011
		#elif __STDC_VERSION__ == 201710L
			#define CMN_LANGUAGE_C_17 1
			#define CMN_LANGUAGE_VERSION 2017
		#elif __STDC_VERSION__ == 202311L
			#define CMN_LANGUAGE_C_23 1
			#define CMN_LANGUAGE_VERSION 2023
		#else
			#define CMN_LANGUAGE_C_90 1
			#define CMN_LANGUAGE_VERSION 1990
		#endif
	#else
		#ifndef __STDC__
			#define CMN_LANGUAGE_C_99 1
			#define CMN_LANGUAGE_VERSION 1999
		#elif !defined(__STDC_VERSION__)
			#define CMN_LANGUAGE_C_99 1
			#define CMN_LANGUAGE_VERSION 1999
			#define CMN_LANGUAGE_MS_EXTENSIONS 1
		#elif __STDC_VERSION__ == 201112L
			#define CMN_LANGUAGE_C_11 1
			#define CMN_LANGUAGE_VERSION 2011
			#define CMN_LANGUAGE_MS_EXTENSIONS 1
		#elif __STDC_VERSION__ == 201710L
			#define CMN_LANGUAGE_C_17 1
			#define CMN_LANGUAGE_VERSION 2017
			#define CMN_LANGUAGE_MS_EXTENSIONS 1
		#else /* unknown msvc version and language standatd */
			#define CMN_LANGUAGE_C_OLD 1
			#define CMN_LANGUAGE_VERSION 0
		#endif
	#endif
#endif

#ifdef __OBJC__
	// Source language is Objective-C.
	#define CMN_LANGUAGE_OBJECTIVEC 1

	#if defined(CMN_LANGUAGE_CPP)
		// Source language is Objective-C++.
		#define CMN_LANGUAGE_OBJECTIVECPP 1
	#endif
#endif

#endif // CMN_COMMONLANGUAGE_H

