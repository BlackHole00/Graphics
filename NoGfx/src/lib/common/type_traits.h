#ifndef CMN_TYPETRAITS_H
#define CMN_TYPETRAITS_H

#include <assert.h>
#include <string.h>
#include <lib/common/common.h>

// Comparison result
typedef enum CmnCmp {
	// Left operand is less than right operand.
	CMN_LESS	= -1,
	// Left operand is equal to right operand.
	CMN_EQUALS	=  0,
	// Left operand is greater than right operand.
	CMN_MORE	=  1,
} CmnCmp;

template <typename T>
struct CmnTypeTraitsStaticAssert {
	static const bool value = false;
};

inline size_t cmnHashInteger64(uint64_t value) {
	uint64_t hash = value + 0x9e3779b97f4a7c15ULL;
	hash = (hash ^ (hash >> 30)) * 0xbf58476d1ce4e5b9ULL;
	hash = (hash ^ (hash >> 27)) * 0x94d049bb133111ebULL;
	hash = hash ^ (hash >> 31);

	return (size_t)hash;
}

// Type-specific operations for generic algorithms and data structures.
template <typename T>
struct CmnTypeTraits {
	// eq checks equality for type T.
	static bool eq(const T& left, const T& right) {
		(void)left; (void)right;

		static_assert(CmnTypeTraitsStaticAssert<T>::value, "Using type traits of an unknown type. Please implement CmnTypeTraits<T> for your custom data type.");
	}

	// cmp compares two values of type T.
	static CmnCmp cmp(const T& left, const T& right) {
		(void)left; (void)right;

		static_assert(CmnTypeTraitsStaticAssert<T>::value, "Using type traits of an unknown type. Please implement CmnTypeTraits<T> for your custom data type.");
	}

	// hash computes the hash code for type T.
	static size_t hash(const T& value) {
		(void)value;

		static_assert(CmnTypeTraitsStaticAssert<T>::value, "Using type traits of an unknown type. Please implement CmnTypeTraits<T> for your custom data type.");
	}
};

// Checks equality using CmnTypeTraits.
template <typename T>
bool cmnEq(const T& left, const T& right) {
	return CmnTypeTraits<T>::eq(left, right);
}

// Compares two values using CmnTypeTraits.
template <typename T>
CmnCmp cmnCmp(const T& left, const T& right) {
	return CmnTypeTraits<T>::cmp(left, right);
}

// Hashes a value using CmnTypeTraits.
template <typename T>
size_t cmnHash(const T& value) {
	return CmnTypeTraits<T>::hash(value);
}

// Define default CmnTypeTraits specializations for primitive comparable types.
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
	static size_t hash(const _T& value) {			\
		return cmnHashInteger64((uint64_t)value);	\
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


template <>
struct CmnTypeTraits<float> {
	static bool eq(const float& left, const float& right) {
		return left == right;
	}

	static CmnCmp cmp(const float& left, const float& right) {
		if (left == right) {
			return CMN_EQUALS;
		} else if (left > right) {
			return CMN_MORE;
		}
		return CMN_LESS;
	}

	static size_t hash(const float& value) {
		float normalized = (value == 0.0f) ? 0.0f : value;
		uint32_t bits;
		memcpy(&bits, &normalized, sizeof(bits));
		return cmnHashInteger64((uint64_t)bits);
	}
};

template <>
struct CmnTypeTraits<double> {
	static bool eq(const double& left, const double& right) {
		return left == right;
	}

	static CmnCmp cmp(const double& left, const double& right) {
		if (left == right) {
			return CMN_EQUALS;
		} else if (left > right) {
			return CMN_MORE;
		}
		return CMN_LESS;
	}

	static size_t hash(const double& value) {
		double normalized = (value == 0.0) ? 0.0 : value;
		uint64_t bits;
		memcpy(&bits, &normalized, sizeof(bits));
		return cmnHashInteger64(bits);
	}
};

#endif

