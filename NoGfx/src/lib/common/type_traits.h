#ifndef CMN_TYPETRAITS_H
#define CMN_TYPETRAITS_H

#include <assert.h>
#include <lib/common/common.h>

typedef enum CmnCmp {
	CMN_LESS	= -1,
	CMN_EQUALS	=  0,
	CMN_MORE	=  1,
} CmnCmp;

template <typename T>
struct CmnTypeTraits {
	static bool eq(const T& left, const T& right) {
		(void)left; (void)right;

		static_assert(false, "Using type traits of an unknown type. Please implement CmnTypeTraits<T> for your custom data type.");
	}

	static CmnCmp cmp(const T& left, const T& right) {
		(void)left; (void)right;

		static_assert(false, "Using type traits of an unknown type. Please implement CmnTypeTraits<T> for your custom data type.");
	}
};

template <typename T>
bool cmnEq(const T& left, const T& right) {
	return CmnTypeTraits<T>::eq(left, right);
}

template <typename T>
CmnCmp cmnCmp(const T& left, const T& right) {
	return CmnTypeTraits<T>::cmp(left, right);
}

#define CMN_DEFINE_DEFAULT_TYPETRAITS_FOR(_T)			\
template <>							\
struct CmnTypeTraits<_T> {					\
	static bool eq(const _T& left, const _T& right) {	\
		return left == right;				\
	}							\
	static CmnCmp cmp(const _T& left, const _T& right) {	\
		if (left == right) {				\
			return CMN_EQUALS;			\
		} else if (left > right) {			\
			return CMN_MORE;			\
		} else {					\
			return CMN_LESS;			\
		}						\
	}							\
}

CMN_DEFINE_DEFAULT_TYPETRAITS_FOR(uint8_t);
CMN_DEFINE_DEFAULT_TYPETRAITS_FOR(int8_t);
CMN_DEFINE_DEFAULT_TYPETRAITS_FOR(uint16_t);
CMN_DEFINE_DEFAULT_TYPETRAITS_FOR(int16_t);
CMN_DEFINE_DEFAULT_TYPETRAITS_FOR(uint32_t);
CMN_DEFINE_DEFAULT_TYPETRAITS_FOR(int32_t);
CMN_DEFINE_DEFAULT_TYPETRAITS_FOR(uint64_t);
CMN_DEFINE_DEFAULT_TYPETRAITS_FOR(int64_t);
CMN_DEFINE_DEFAULT_TYPETRAITS_FOR(uintptr_t);

CMN_DEFINE_DEFAULT_TYPETRAITS_FOR(float);
CMN_DEFINE_DEFAULT_TYPETRAITS_FOR(double);

#endif

