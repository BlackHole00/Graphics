#ifndef CMN_TYPETRAITS_H
#define CMN_TYPETRAITS_H

#include <assert.h>
#include <lib/common/common.h>

/**
	Comparison result used by CmnTypeTraits.
*/
typedef enum CmnCmp {
	/** Left operand is less than right operand. */
	CMN_LESS	= -1,
	/** Left operand is equal to right operand. */
	CMN_EQUALS	=  0,
	/** Left operand is greater than right operand. */
	CMN_MORE	=  1,
} CmnCmp;

/**
	Type trait specialization point for equality and ordering.
*/
template <typename T>
struct CmnTypeTraits {
	/**
		Checks equality between two values of type `T`.

		@param left Left operand.
		@param right Right operand.

		@return True when values are equal.
	*/
	static bool eq(const T& left, const T& right) {
		(void)left; (void)right;

		static_assert(false, "Using type traits of an unknown type. Please implement CmnTypeTraits<T> for your custom data type.");
	}

	/**
		Compares two values of type `T`.

		@param left Left operand.
		@param right Right operand.

		@return `CMN_LESS`, `CMN_EQUALS`, or `CMN_MORE`.
	*/
	static CmnCmp cmp(const T& left, const T& right) {
		(void)left; (void)right;

		static_assert(false, "Using type traits of an unknown type. Please implement CmnTypeTraits<T> for your custom data type.");
	}
};

/**
	Checks equality between two values using CmnTypeTraits.

	@param left Left operand.
	@param right Right operand.

	@return True when values are equal.
*/
template <typename T>
bool cmnEq(const T& left, const T& right) {
	return CmnTypeTraits<T>::eq(left, right);
}

/**
	Compares two values using CmnTypeTraits.

	@param left Left operand.
	@param right Right operand.

	@return `CMN_LESS`, `CMN_EQUALS`, or `CMN_MORE`.
*/
template <typename T>
CmnCmp cmnCmp(const T& left, const T& right) {
	return CmnTypeTraits<T>::cmp(left, right);
}

/**
	Defines a default CmnTypeTraits specialization for primitive comparable types.
*/
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

