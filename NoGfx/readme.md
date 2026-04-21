# NoGfx Common Utilities

## CmnTypeTraits

`CmnTypeTraits<T>` defines operations used by generic data structures and algorithms:

- `eq(const T&, const T&) -> bool`
- `cmp(const T&, const T&) -> CmnCmp`
- `hash(const T&) -> size_t`

Helpers are provided for convenience:

- `cmnEq(left, right)`
- `cmnCmp(left, right)`
- `cmnHash(value)`

Default specializations are available for primitive numeric types.

For custom types, provide a specialization:

```cpp
typedef struct MyKey {
	uint32_t a;
	uint32_t b;
} MyKey;

template <>
struct CmnTypeTraits<MyKey> {
	static bool eq(const MyKey& left, const MyKey& right) {
		return left.a == right.a && left.b == right.b;
	}

	static CmnCmp cmp(const MyKey& left, const MyKey& right) {
		if (left.a == right.a && left.b == right.b) {
			return CMN_EQUALS;
		}
		if (left.a < right.a || (left.a == right.a && left.b < right.b)) {
			return CMN_LESS;
		}
		return CMN_MORE;
	}

	static size_t hash(const MyKey& value) {
		uint64_t packed = ((uint64_t)value.a << 32) | value.b;
		return cmnHashInteger64(packed);
	}
};
```

## CmnHashMap

`CmnHashMap<K, V>` is a generic open-addressing hash map that uses:

- `CmnTypeTraits<K>::hash` to hash keys.
- `CmnTypeTraits<K>::eq` to compare keys.

Header: `src/lib/common/hash_map.h`

Main API:

- `cmnCreateHashMap`
- `cmnDestroyHashMap`
- `cmnReserve`
- `cmnInsert`
- `cmnGet`
- `cmnContains`
- `cmnRemove`

Behavior notes:

- Insert overwrites existing values for the same key.
- Lookup returns `defaultValue` when the key is missing.
- Buckets grow automatically as load factor increases.

Minimal usage:

```cpp
CmnHashMap<int32_t, int32_t> map;
CmnResult result;

cmnCreateHashMap(&map, 16, -1, cmnHeapAllocator(), &result);
cmnInsert(&map, 42, 9001, nullptr);

bool found = false;
int32_t value = cmnGet(&map, 42, &found);

cmnDestroyHashMap(&map);
```
