template <typename T>
void cmnCreateElementPool(CmnElementPool<T>* pool, CmnAllocator backingAllocator, CmnResult* result) {
	CmnResult localResult;

	*pool = {};

	cmnCreateExponentialArray(&pool->elements, backingAllocator, &localResult);
	if (localResult != CMN_SUCCESS) {
		CMN_SET_RESULT(result, localResult);
		return;
	}

	cmnCreateExponentialArray(&pool->freeList, backingAllocator, &localResult);
	if (localResult != CMN_SUCCESS) {
		CMN_SET_RESULT(result, localResult);
		return;
	}

	CMN_SET_RESULT(result, CMN_SUCCESS);
}

template <typename T>
size_t cmnInsert(CmnElementPool<T>* pool, const T& element, CmnResult* result) {
	CmnResult localResult;

	if (pool->freeList.length > 0) {
		size_t index = cmnLast(&pool->freeList);
		cmnPop(&pool->freeList);

		pool->elements[index] = element;
		pool->length++;

		CMN_SET_RESULT(result, CMN_SUCCESS);
		return index;
	}

	cmnAppend(&pool->elements, element, &localResult);
	if (localResult != CMN_SUCCESS) {
		CMN_SET_RESULT(result, localResult);
		return SIZE_T_MAX;
	}

	pool->length++;

	CMN_SET_RESULT(result, CMN_SUCCESS);
	return pool->elements.length - 1;
}

template <typename T>
void cmnRemove(CmnElementPool<T>* pool, size_t index) {
	if (index >= pool->elements.length) {
		return;
	}

	pool->elements[index] = {};
	cmnAppend(&pool->freeList, index, nullptr);
}

template <typename T>
T& cmnGet(CmnElementPool<T>* pool, size_t index) {
	assert(index < pool->length);

	return pool->elements[index];
}

template <typename T>
void cmnSet(CmnElementPool<T>* pool, size_t index, const T& element) {
	if (index >= pool->elements.length) {
		return;
	}

	pool->elements[index] = element;
}
