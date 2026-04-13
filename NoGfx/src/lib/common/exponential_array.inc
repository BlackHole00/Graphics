template <typename T, size_t N>
void cmnCreateExponentialArray(CmnExponentialArray<T, N>* array, CmnAllocator backingAllocator, CmnResult* result) {
	array->backingAllocator	= backingAllocator;
	memset(array->buckets, 0, sizeof(array->buckets));
	array->length		= 0;
	array->last_filled_bucket = 0;

	array->buckets[0]	= cmnAlloc<T>(array->backingAllocator, CMN_EXPONENTIALARRAY_BASE_BUCKET_LEN, result);
}

template <typename T, size_t N>
bool cmnAppend(CmnExponentialArray<T, N>* array, const T& value, CmnResult* result) {
	size_t idx = array->length;

	if (!cmnResize(array, array->length + 1, result)) {
		return false;
	}

	cmnSet<T, N>(array, idx, value);
	return true;
}

template <typename T, size_t N>
bool cmnResize(CmnExponentialArray<T, N>* array, size_t length, CmnResult* result) {
	if (array->length >= length) {
		CMN_SET_RESULT(result, CMN_SUCCESS);
		return true;
	}

	size_t bucketIndex, elementIndex;
	cmnDecomposeExponentialArrayIndex(length, &bucketIndex, &elementIndex);

	if (bucketIndex >= N) {
		CMN_SET_RESULT(result, CMN_OUT_OF_MEMORY);
		return false;
	}

	if (bucketIndex <= array->last_filled_bucket) {
		array->length = length;

		CMN_SET_RESULT(result, CMN_SUCCESS);
		return true;
	}

	for (size_t i = array->last_filled_bucket; i <= bucketIndex; i++) {
		if (array->buckets[i] != nullptr) {
			continue;
		}

		array->buckets[i] = cmnAlloc<T>(
			array->backingAllocator,
			1 << (CMN_EXPONENTIALARRAY_BASE_BUCKET_LEN_BITS + i - 1),
			result
		);
		if (array->buckets[i] == nullptr) {
			return false;
		}

		array->last_filled_bucket = i;
	}

	array->length = length;

	CMN_SET_RESULT(result, CMN_SUCCESS);
	return true;
}

template <typename T, size_t N>
void cmnSet(CmnExponentialArray<T, N>* array, size_t index, const T& value) {
	assert(index <= array->length);

	size_t bucketIndex, elementIndex;
	cmnDecomposeExponentialArrayIndex(index, &bucketIndex, &elementIndex);

	array->buckets[bucketIndex][elementIndex] = value;
}

template <typename T, size_t N>
T& cmnGet(CmnExponentialArray<T, N>* array, size_t index) {
	assert(index <= array->length);

	size_t bucketIndex, elementIndex;
	cmnDecomposeExponentialArrayIndex(index, &bucketIndex, &elementIndex);

	return array->buckets[bucketIndex][elementIndex];
}

inline void cmnDecomposeExponentialArrayIndex(size_t index, size_t* bucketIndex, size_t* elementIndex) {
	size_t start = CMN_EXPONENTIALARRAY_BASE_BUCKET_LEN_BITS;

	size_t indexShift = index >> start;

	if (indexShift == 0) {
		*bucketIndex = 0;
	} else {
		*bucketIndex = CMN_EXPONENTIALARRAY_POINTER_SIZE - __builtin_clzll(indexShift);
	}

	if (*bucketIndex == 0) {
		*elementIndex = index;
	} else {
		size_t bucketBits = start + *bucketIndex - 1;
		*elementIndex = index & ((1ULL << bucketBits) - 1);
	}
}


