template <typename T, size_t N>
static CmnChainNode<T, N>* cmninternalChainAllocateNode(CmnAllocator allocator, CmnResult* result) {
	CmnResult localResult;

	CmnChainNode<T, N>* node = cmnAlloc<CmnChainNode<T, N>>(allocator, &localResult);
	if (localResult != CMN_SUCCESS) {
		CMN_SET_RESULT(result, localResult);
		return nullptr;
	}

	*node = {};
	CMN_SET_RESULT(result, CMN_SUCCESS);
	return node;
}

template <typename T, size_t N>
void cmnCreateChain(CmnChain<T, N>* chain) {
	*chain = {};
}

template <typename T, size_t N>
void cmnDestroyChain(CmnChain<T, N>* chain, CmnAllocator allocator) {
	CmnChainNode<T, N>* node = chain->head;
	while (node != nullptr) {
		CmnChainNode<T, N>* nextNode = node->next;
		cmnFree(allocator, node);
		node = nextNode;
	}

	chain->head = nullptr;
}

template <typename T, size_t N>
void cmnInsert(CmnChain<T, N>* chain, const T& value, CmnAllocator allocator, CmnResult* result) {
	CmnScopedWriteRWMutex guard(&chain->mutex);

	CmnChainNode<T, N>* node = chain->head;
	CmnChainNode<T, N>* lastNode = nullptr;
	while (node != nullptr && node->count == N) {
		lastNode = node;
		node = node->next;
	}

	if (node == nullptr) {
		CmnChainNode<T, N>* newNode = cmninternalChainAllocateNode<T, N>(allocator, result);
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

	node->elements[node->count] = value;
	node->count++;
	CMN_SET_RESULT(result, CMN_SUCCESS);
}

template <typename T, size_t N>
void cmnRemove(CmnChain<T, N>* chain, const T& value, CmnAllocator allocator) {
	CmnScopedWriteRWMutex guard(&chain->mutex);

	CmnChainNode<T, N>* previousNode = nullptr;
	CmnChainNode<T, N>* node = chain->head;

	while (node != nullptr) {
		for (uint16_t i = 0; i < node->count; i++) {
			if (!cmnEq(node->elements[i], value)) {
				continue;
			}

			uint16_t lastIndex = node->count - 1;
			if (i != lastIndex) {
				node->elements[i] = node->elements[lastIndex];
			}
			node->elements[lastIndex] = {};
			node->count--;

			if (node->count == 0) {
				CmnChainNode<T, N>* nextNode = node->next;
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

template <typename T, size_t N>
bool cmnContains(CmnChain<T, N>* chain, const T& value) {
	CmnScopedReadRWMutex guard(&chain->mutex);

	CmnChainNode<T, N>* node = chain->head;
	while (node != nullptr) {
		for (uint16_t i = 0; i < node->count; i++) {
			if (cmnEq(node->elements[i], value)) {
				return true;
			}
		}

		node = node->next;
	}

	return false;
}

template <typename T, size_t N>
void cmnCreateChainIterator(CmnChain<T, N>* chain, CmnChainIterator<T, N>* iter) {
	iter->chain = chain;
	iter->currentNode = chain->head;
	iter->currentElementInNode = 0;
}

template <typename T, size_t N>
bool cmnIterate(CmnChainIterator<T, N>* iter, T** value) {
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

	*value = &iter->currentNode->elements[iter->currentElementInNode];
	iter->currentElementInNode++;

	if (iter->currentElementInNode >= iter->currentNode->count) {
		iter->currentNode = iter->currentNode->next;
		iter->currentElementInNode = 0;
	}

	return true;
}