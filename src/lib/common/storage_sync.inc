template <typename T>
T* cmnStorageSyncAcquireResource(CmnHandleMap<T>* map, CmnStorageSync* sync, CmnHandle handle, bool* wasHandleValid) {
	bool localWasHandleValid;

	for (;;) {
		cmnStorageSyncWaitUntilNormal(sync);

		CmnScopedReadRWMutex guard(&sync->mutex);
		CmnStorageSyncState state = (CmnStorageSyncState)cmnAtomicLoad(&sync->state.value, CMN_ACQUIRE);
		if (state != CMN_STORAGE_SYNC_NORMAL) {
			continue;
		}

		T& resource = cmnGet(map, handle, &localWasHandleValid);
		if (!localWasHandleValid) {
			if (wasHandleValid != nullptr) {
				*wasHandleValid = false;
			}

			return &resource;
		}

		cmnStorageSyncMarkAsUsingResources(sync);

		if (wasHandleValid != nullptr) {
			*wasHandleValid = true;
		}

		return &resource;
	}
}

template <typename T>
T* cmnStorageSyncAcquireResource(CmnPointerMap<T>* map, CmnStorageSync* sync, uintptr_t ptr, bool* wasHandleValid) {
	bool localWasHandleValid;

	for (;;) {
		cmnStorageSyncWaitUntilNormal(sync);

		CmnScopedReadRWMutex guard(&sync->mutex);
		CmnStorageSyncState state = (CmnStorageSyncState)cmnAtomicLoad(&sync->state.value, CMN_ACQUIRE);
		if (state != CMN_STORAGE_SYNC_NORMAL) {
			continue;
		}

		T& resource = cmnGet(map, ptr, &localWasHandleValid);
		if (!localWasHandleValid) {
			if (wasHandleValid != nullptr) {
				*wasHandleValid = false;
			}

			return &resource;
		}

		cmnStorageSyncMarkAsUsingResources(sync);

		if (wasHandleValid != nullptr) {
			*wasHandleValid = true;
		}

		return &resource;
	}
}

