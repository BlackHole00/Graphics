template <typename T, size_t N>
bool cmninternalIsValid(const CmnStaticHandleMap<T, N>* map, CmnHandle handle) {
	return handle.index < N &&
		map->buckets[handle.index].isInUse &&
		handle.generation == map->buckets[handle.index].generation;
}

template <typename T, size_t N>
uint32_t cmninternalHandleMapGetNextFreeBucket(CmnStaticHandleMap<T, N>* map, CmnResult* result) {
	uint32_t firstFreeBucketIndex = map->firstFree;
	if (firstFreeBucketIndex == UINT32_MAX) {
		CMN_SET_RESULT(result, CMN_OUT_OF_RESOURCE_SLOTS);
		return UINT32_MAX;
	}

	// Walk free list, skipping buckets whose generation is UINT32_MAX.
	while (firstFreeBucketIndex < N) {
		CmnHandleMapBucket<T>* bucket = &map->buckets[firstFreeBucketIndex];

		if (bucket->generation == UINT32_MAX) {
			firstFreeBucketIndex = bucket->nextFreeIndex;
		} else {
			break;
		}
	}

	if (firstFreeBucketIndex >= N) {
		CMN_SET_RESULT(result, CMN_OUT_OF_RESOURCE_SLOTS);
		return UINT32_MAX;
	}

	CmnHandleMapBucket<T>* bucket = &map->buckets[firstFreeBucketIndex];
	map->firstFree = bucket->nextFreeIndex;

	CMN_SET_RESULT(result, CMN_SUCCESS);
	return firstFreeBucketIndex;
}

template <typename T, size_t N>
void cmnCreateStaticHandleMap(CmnStaticHandleMap<T, N>* map, T defaultElement) {
	for (uint32_t i = 0; i < N; ++i) {
		map->buckets[i].isInUse = false;
		map->buckets[i].generation = 0;
		map->buckets[i].nextFreeIndex = (i + 1 < N) ? (i + 1) : UINT32_MAX;
	}

	map->firstFree = (N > 0) ? 0 : UINT32_MAX;
	map->defaultElement = defaultElement;
}

template <typename T, size_t N>
CmnHandle cmnInsert(CmnStaticHandleMap<T, N>* map, const T& element, CmnResult* result) {
	CmnResult localResult;

	uint32_t freeBucketIndex = cmninternalHandleMapGetNextFreeBucket(map, &localResult);
	if (localResult != CMN_SUCCESS) {
		CMN_SET_RESULT(result, localResult);
		return {};
	}

	CmnHandleMapBucket<T>* bucket = &map->buckets[freeBucketIndex];
	bucket->generation++;
	bucket->isInUse = true;
	bucket->element = element;

	CMN_SET_RESULT(result, CMN_SUCCESS);
	return CmnHandle{
		/*index=*/ freeBucketIndex,
		/*generation=*/ bucket->generation,
	};
}

template <typename T, size_t N>
bool cmnIsValid(const CmnStaticHandleMap<T, N>* map, CmnHandle handle) {
	return cmninternalIsValid<T, N>(map, handle);
}

template <typename T, size_t N>
T& cmnGet(CmnStaticHandleMap<T, N>* map, CmnHandle handle, bool* wasHandleValid) {
	if (!cmninternalIsValid(map, handle)) {
		if (wasHandleValid != nullptr) {
			*wasHandleValid = false;
		}

		return map->defaultElement;
	} else {
		if (wasHandleValid != nullptr) {
			*wasHandleValid = true;
		}

		return map->buckets[handle.index].element;
	}
}

template <typename T, size_t N>
void cmnRemove(CmnStaticHandleMap<T, N>* map, CmnHandle handle) {
	if (!cmninternalIsValid(map, handle)) {
		return;
	}

	CmnHandleMapBucket<T>* bucket = &map->buckets[handle.index];

	bucket->isInUse = false;
	bucket->nextFreeIndex = map->firstFree;

	map->firstFree = handle.index;
}
