// A defer implementation for C++11, based from the article http://www.gingerbill.org/article/2015/08/19/defer-in-cpp/.  
// Please note that this hack requires an optimizing compiler to produce good codegen.
// 
// Gcc 15.2 with optimization level O2 inlines both the constructor and the destructor calls and does not create a
// thunk function for the resolution of the defer body: https://godbolt.org/z/ddM4K4axr.  
// The same applies for clang.

#ifndef CMN_DEFER_H
#define CMN_DEFER_H

template <typename F>
struct CmnInternalDefer {
	F f;
	CmnInternalDefer(F f) : f(f) {}
	~CmnInternalDefer() { f(); }
};

template <typename F>
CmnInternalDefer<F> cmnInternalDeferFunc(F f) {
	return CmnInternalDefer<F>(f);
}

#define CMN_INTERNAL_DEFER_1(x, y)	x##y
#define CMN_INTERNAL_DEFER_2(x, y)	CMN_INTERNAL_DEFER_1(x, y)
#define CMN_INTERNAL_DEFER_3(x)		CMN_INTERNAL_DEFER_2(x, __COUNTER__)

#define CMN_DEFER(...)	auto CMN_INTERNAL_DEFER_3(_defer_) = cmnInternalDeferFunc([&](){__VA_ARGS__;})

#ifndef CMN_AVOID_POLLUTING_DEFAULT_NAMESPACE
	#define DEFER(...)	CMN_DEFER(__VA_ARGS__)
	#define defer(...)	CMN_DEFER(__VA_ARGS__)
#endif

#endif // CMN_DEFER_H
