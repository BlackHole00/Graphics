template <typename K, typename V, size_t N>
static CmnKeyedChainNode<K, V, N>* cmninternalKeyedChainAllocateNode(CmnAllocator allocator, CmnResult* result) {
	CmnResult localResult;

	CmnKeyedChainNode<K, V, N>* node = cmnAlloc<CmnKeyedChainNode<K, V, N>>(allocator, &localResult);
	if (localResult != CMN_SUCCESS) {
		CMN_SET_RESULT(result, localResult);
		return nullptr;
	}

	*node = {};
	CMN_SET_RESULT(result, CMN_SUCCESS);
	return node;
}

template <typename K, typename V, size_t N>
void cmnCreateKeyedChain(CmnKeyedChain<K, V, N>* chain, const V& defaultValue) {
	*chain = {};
	chain->defaultValue = defaultValue;
}

template <typename K, typename V, size_t N>
void cmnDestroyKeyedChain(CmnKeyedChain<K, V, N>* chain, CmnAllocator allocator) {
	CmnKeyedChainNode<K, V, N>* node = chain->head;
	while (node != nullptr) {
		CmnKeyedChainNode<K, V, N>* nextNode = node->next;
		cmnFree(allocator, node);
		node = nextNode;
	}

	chain->defaultValue = {};
	chain->head = nullptr;
}

template <typename K, typename V, size_t N>
void cmnInsert(CmnKeyedChain<K, V, N>* chain, const K& key, const V& value, CmnAllocator allocator, CmnResult* result) {
	CmnScopedWriteRWMutex guard(&chain->mutex);

	CmnKeyedChainNode<K, V, N>* node = chain->head;
	CmnKeyedChainNode<K, V, N>* lastNode = nullptr;
	while (node != nullptr) {
		for (uint16_t i = 0; i < node->count; i++) {
			if (cmnEq(node->keys[i], key)) {
				node->values[i] = value;
				CMN_SET_RESULT(result, CMN_SUCCESS);
				return;
			}
		}

		if (node->count < N) {
			break;
		}

		lastNode = node;
		node = node->next;
	}

	if (node == nullptr) {
		CmnKeyedChainNode<K, V, N>* newNode = cmninternalKeyedChainAllocateNode<K, V, N>(allocator, result);
		if (newNode == nullptr) {
			return;
		}

		if (lastNode == nullptr) {
			chain->head = newNode;
		} else {
			lastNode->next = newNode;
		}

		node = newNode;
	}

	node->keys[node->count] = key;
	node->values[node->count] = value;
	node->count++;
	CMN_SET_RESULT(result, CMN_SUCCESS);
}

template <typename K, typename V, size_t N>
void cmnRemove(CmnKeyedChain<K, V, N>* chain, const K& key, CmnAllocator allocator) {
	CmnScopedWriteRWMutex guard(&chain->mutex);

	CmnKeyedChainNode<K, V, N>* previousNode = nullptr;
	CmnKeyedChainNode<K, V, N>* node = chain->head;

	while (node != nullptr) {
		for (uint16_t i = 0; i < node->count; i++) {
			if (!cmnEq(node->keys[i], key)) {
				continue;
			}

			uint16_t lastIndex = node->count - 1;
			if (i != lastIndex) {
				node->keys[i] = node->keys[lastIndex];
				node->values[i] = node->values[lastIndex];
			}
			node->keys[lastIndex] = {};
			node->values[lastIndex] = {};
			node->count--;

			if (node->count == 0) {
				CmnKeyedChainNode<K, V, N>* nextNode = node->next;
				if (previousNode == nullptr) {
					chain->head = nextNode;
				} else {
					previousNode->next = nextNode;
				}

				cmnFree(allocator, node);
			}

			return;
		}

		previousNode = node;
		node = node->next;
	}
}

template <typename K, typename V, size_t N>
bool cmnContains(CmnKeyedChain<K, V, N>* chain, const K& key) {
	CmnScopedReadRWMutex guard(&chain->mutex);

	CmnKeyedChainNode<K, V, N>* node = chain->head;
	while (node != nullptr) {
		for (uint16_t i = 0; i < node->count; i++) {
			if (cmnEq(node->keys[i], key)) {
				return true;
			}
		}

		node = node->next;
	}

	return false;
}

template <typename K, typename V, size_t N>
V& cmnGet(CmnKeyedChain<K, V, N>* chain, const K& key, bool* found) {
	CmnScopedReadRWMutex guard(&chain->mutex);

	CmnKeyedChainNode<K, V, N>* node = chain->head;
	while (node != nullptr) {
		for (uint16_t i = 0; i < node->count; i++) {
			if (cmnEq(node->keys[i], key)) {
				if (found != nullptr) {
					*found = true;
				}

				return node->values[i];
			}
		}

		node = node->next;
	}

	if (found != nullptr) {
		*found = false;
	}

	return chain->defaultValue;
}

template <typename K, typename V, size_t N>
void cmnCreateKeyedChainIterator(CmnKeyedChain<K, V, N>* chain, CmnKeyedChainIterator<K, V, N>* iter) {
	iter->chain = chain;
	iter->currentNode = chain->head;
	iter->currentElementInNode = 0;
}

template <typename K, typename V, size_t N>
bool cmnIterate(CmnKeyedChainIterator<K, V, N>* iter, K** key, V** value) {
	if (iter->currentNode == nullptr) {
		return false;
	}

	if (iter->currentElementInNode >= iter->currentNode->count) {
		iter->currentNode = iter->currentNode->next;
		iter->currentElementInNode = 0;
		if (iter->currentNode == nullptr) {
			return false;
		}
	}

	*key = &iter->currentNode->keys[iter->currentElementInNode];
	*value = &iter->currentNode->values[iter->currentElementInNode];
	iter->currentElementInNode++;

	if (iter->currentElementInNode >= iter->currentNode->count) {
		iter->currentNode = iter->currentNode->next;
		iter->currentElementInNode = 0;
	}

	return true;
}
