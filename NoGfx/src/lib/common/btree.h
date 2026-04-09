#ifndef CMN_BTREE_H
#define CMN_BTREE_H

#include <lib/common/common.h>
#include <lib/common/type_traits.h>
#include <lib/common/pool.h>
#include <lib/common/algorithms.h>

#define CMN_BTREE_ORDER	16
#define CMN_BTREE_T	8

template <typename K, typename V>
struct CmnBTreeNode {
	bool		isLeaf;
	uint8_t		keyCount;

	K			keys	[CMN_BTREE_ORDER - 1];
	V			elements[CMN_BTREE_ORDER - 1];
	CmnBTreeNode<K, V>*	children[CMN_BTREE_ORDER];
};

template <typename K, typename V>
struct CmnBTree {
	CmnPool*		nodesPool;

	CmnBTreeNode<K, V>*	root;
	V			defaultElement;
};

template <typename K, typename V>
void cmnCreateBTree(CmnBTree<K, V>* tree, const V& defaultElement, CmnPool* nodesPool, CmnResult* result) {
	assert(sizeof(CmnBTreeNode<K, V>) <= nodesPool->blockSize);

	CmnResult localResult;

	CmnBTreeNode<K, V>* root = cmnPoolAlloc<CmnBTreeNode<K, V>>(nodesPool, &localResult);
	if (localResult != CMN_SUCCESS) {
		CMN_SET_RESULT(result, localResult);
		return;
	}
	root->isLeaf = true;

	tree->root = root;
	tree->nodesPool = nodesPool;
	tree->defaultElement = defaultElement;

	CMN_SET_RESULT(result, CMN_SUCCESS);
}

template <typename K, typename V>
void cmnBTreeSplit(CmnBTree<K, V>* tree, CmnBTreeNode<K, V>* node, size_t index, CmnResult* result) {
	CmnResult localResult;

	CmnBTreeNode<K, V>* left = node->children[index];
	CmnBTreeNode<K, V>* right = cmnPoolAlloc<CmnBTreeNode<K, V>>(tree->nodesPool, &localResult);
	if (localResult != CMN_SUCCESS) {
		CMN_SET_RESULT(result, localResult);
		return;
	}

	K medianKey	= left->keys[CMN_BTREE_T - 1];
	V medianElement	= left->elements[CMN_BTREE_T - 1];

	right->isLeaf	= left->isLeaf;
	right->keyCount	= CMN_BTREE_T - 1;
	left->keyCount	= CMN_BTREE_T - 1;

	for (size_t i = 0; i < CMN_BTREE_T - 1; i++) {
		right->keys[i]		= left->keys[i + CMN_BTREE_T];
		right->elements[i]	= left->elements[i + CMN_BTREE_T];
	}

	if (!left->isLeaf) {
		for (size_t i = 0; i < CMN_BTREE_T; i++) {
			right->children[i] = left->children[i + CMN_BTREE_T];
		}
	}

	cmnInsertAtIndex(node->children, node->keyCount + 1, index + 1, right);
	cmnInsertAtIndex(node->keys, node->keyCount, index, medianKey);
	cmnInsertAtIndex(node->elements, node->keyCount, index, medianElement);

	node->keyCount += 1;

	CMN_SET_RESULT(result, CMN_SUCCESS);
}

template <typename K, typename V>
void cmnBTreeInsertNotNull(
	CmnBTree<K, V>* tree,
	CmnBTreeNode<K, V>* node,
	const K& key,
	const V& element,
	CmnResult* result
) {
	CmnResult localResult;

	if (node->isLeaf) {
		size_t i = cmnFindFirstGreaterElementIndex(node->keys, node->keyCount, key);

		cmnInsertAtIndex(node->keys, node->keyCount, i, key);
		cmnInsertAtIndex(node->elements, node->keyCount, i, element);
		node->keyCount += 1;
	} else {
		size_t i = cmnFindFirstGreaterElementIndex(node->keys, node->keyCount, key);

		if (node->children[i]->keyCount == CMN_BTREE_ORDER - 1) {
			cmnBTreeSplit(tree, node, i, &localResult);
			if (localResult != CMN_SUCCESS) {
				CMN_SET_RESULT(result, localResult);
				return;
			}

			if (key > node->keys[i]) {
				i += 1;
			}
		}

		cmnBTreeInsertNotNull(tree, node->children[i], key, element, result);
	}

	CMN_SET_RESULT(result, CMN_SUCCESS);
}

template <typename K, typename V>
CmnBTreeNode<K, V>* cmnBTreeSearchNode(CmnBTreeNode<K, V>* node, const K& key, size_t* indexInNode) {
	size_t i = 0;
	while (i < node->keyCount && cmnCmp(node->keys[i], key) == CMN_LESS) {
		i++;
	}

	if (i < node->keyCount && cmnCmp(node->keys[i], key) == CMN_EQUALS) {
		*indexInNode = i;
		return node;
	}

	if (node->isLeaf) {
		return nullptr;
	}

	return cmnBTreeSearchNode(node->children[i], key, indexInNode);
}

template <typename K, typename V>
void cmnBTreePredecessor(CmnBTreeNode<K, V>* node, size_t index, K* key, V* element) {
	CmnBTreeNode<K, V>* current = node->children[index];
	while (!current->isLeaf) {
		current = current->children[current->keyCount];
	}

	*key = current->keys[current->keyCount - 1];
	*element = current->elements[current->keyCount - 1];
}

template <typename K, typename V>
void cmnBTreeSuccessor(CmnBTreeNode<K, V>* node, size_t index, K* key, V* element) {
	CmnBTreeNode<K, V>* current = node->children[index + 1];
	while (!current->isLeaf) {
		current = current->children[0];
	}

	*key = current->keys[0];
	*element = current->elements[0];
}

template <typename K, typename V>
void cmnBTreeBorrowFromPrev(CmnBTreeNode<K, V>* node, size_t index) {
	// Move an element from left to right
	CmnBTreeNode<K, V>* left = node->children[index - 1];
	CmnBTreeNode<K, V>* right = node->children[index];

	cmnInsertAtIndex(right->keys, right->keyCount, 0, node->keys[index - 1]);
	cmnInsertAtIndex(right->elements, right->keyCount, 0, node->elements[index - 1]);
	if (!right->isLeaf) {
		cmnInsertAtIndex(right->children, right->keyCount + 1, 0, left->children[left->keyCount]);
	}

	node->keys[index - 1] = left->keys[left->keyCount - 1];
	node->elements[index - 1] = left->elements[left->keyCount - 1];

	left->keyCount -= 1;
	right->keyCount += 1;
}

template <typename K, typename V>
void cmnBTreeBorrowFromNext(CmnBTreeNode<K, V>* node, size_t index) {
	// Move an element from right to left
	CmnBTreeNode<K, V>* left = node->children[index];
	CmnBTreeNode<K, V>* right = node->children[index + 1];

	left->keys[left->keyCount]	= node->keys[index];
	left->elements[left->keyCount]	= node->elements[index];
	if (!left->isLeaf) {
		left->children[left->keyCount + 1] = right->children[0];
		cmnRemoveAtIndex(right->children, right->keyCount + 1, 0);
	}

	node->keys[index]	= right->keys[0];
	node->elements[index]	= right->elements[0];

	cmnRemoveAtIndex(right->keys, right->keyCount, 0);
	cmnRemoveAtIndex(right->elements, right->keyCount, 0);
	left->keyCount += 1;
	right->keyCount -= 1;
}

template <typename K, typename V>
void cmnBTreeMerge(CmnBTree<K, V>* tree, CmnBTreeNode<K, V>* node, size_t index) {
	CmnBTreeNode<K, V>* left = node->children[index];
	CmnBTreeNode<K, V>* right = node->children[index + 1];
	
	left->keys[CMN_BTREE_T - 1] = node->keys[index];
	left->elements[CMN_BTREE_T - 1] = node->elements[index];

	for (size_t i = 0; i < right->keyCount; i++) {
		left->keys[CMN_BTREE_T + i] = right->keys[i];
		left->elements[CMN_BTREE_T + i] = right->elements[i];
	}
	if (!right->isLeaf) {
		for (size_t i = 0; i < right->keyCount + 1; i++) {
			left->children[CMN_BTREE_T + i] = right->children[i];
		}
	}

	cmnRemoveAtIndex(node->keys, node->keyCount, index);
	cmnRemoveAtIndex(node->elements, node->keyCount, index);
	cmnRemoveAtIndex(node->children, node->keyCount + 1, index + 1);

	left->keyCount = left->keyCount + right->keyCount + 1;
	node->keyCount -= 1;

	cmnPoolFree(tree->nodesPool, right);
}

template <typename K, typename V>
void cmnBTreeFill(CmnBTree<K, V>* tree, CmnBTreeNode<K, V>* node, size_t index) {
	if (index > 0 && node->children[index - 1]->keyCount >= CMN_BTREE_T) {
		cmnBTreeBorrowFromPrev(node, index);
	} else if (index < node->keyCount && node->children[index + 1]->keyCount >= CMN_BTREE_T) {
		cmnBTreeBorrowFromNext(node, index);
	} else {
		if (index != node->keyCount) {
			cmnBTreeMerge(tree, node, index);
		} else {
			cmnBTreeMerge(tree, node, index - 1);
		}
	}
}

template <typename K, typename V>
void cmnBTreeRemove(CmnBTree<K, V>* tree, CmnBTreeNode<K, V>* node, const K& key);

template <typename K, typename V>
void cmnBTreeRemoveFromNonLeaf(CmnBTree<K, V>* tree, CmnBTreeNode<K, V>* node, size_t index) {
	if (node->children[index]->keyCount >= CMN_BTREE_T) {
		cmnBTreePredecessor(node, index, &node->keys[index], &node->elements[index]);
		cmnBTreeRemove(tree, node->children[index], node->keys[index]);
	} else if (node->children[index + 1]->keyCount >= CMN_BTREE_T) {
		cmnBTreeSuccessor(node, index, &node->keys[index], &node->elements[index]);
		cmnBTreeRemove(tree, node->children[index + 1], node->keys[index]);
	} else {
		K key = node->keys[index];
		cmnBTreeMerge(tree, node, index);
		cmnBTreeRemove(tree, node, key);
	}
}

template <typename K, typename V>
void cmnBTreeRemoveFromLeaf(CmnBTreeNode<K, V>* node, size_t index) {
	cmnRemoveAtIndex(node->elements, node->keyCount, index);
	cmnRemoveAtIndex(node->keys, node->keyCount, index);

	node->keyCount -= 1;
}

template <typename K, typename V>
void cmnBTreeRemove(CmnBTree<K, V>* tree, CmnBTreeNode<K, V>* node, const K& key) {
	size_t i = 0;
	while (i < node->keyCount && cmnCmp(node->keys[i], key) == CMN_LESS) {
		i++;
	}

	if (i < node->keyCount && cmnCmp(node->keys[i], key) == CMN_EQUALS) {
		if (node->isLeaf) {
			cmnBTreeRemoveFromLeaf(node, i);
		} else {
			cmnBTreeRemoveFromNonLeaf(tree, node, i);
		}
	} else {
		if (node->isLeaf) {
			return;
		}

		if (node->children[i]->keyCount < CMN_BTREE_T) {
			cmnBTreeFill(tree, node, i);

			if (i > node->keyCount) {
				i -= 1;
			}
		}

		cmnBTreeRemove(tree, node->children[i], key);
	}
}

template <typename K, typename V>
void cmnInsert(CmnBTree<K, V>* tree, const K& key, const V& element, CmnResult* result) {
	CmnResult localResult;

	if (tree->root->keyCount == CMN_BTREE_ORDER - 1) {
		CmnBTreeNode<K, V>* newRoot = cmnPoolAlloc<CmnBTreeNode<K, V>>(tree->nodesPool, &localResult);
		if (localResult != CMN_SUCCESS) {
			CMN_SET_RESULT(result, localResult);
			return;
		}

		newRoot->children[0] = tree->root;

		cmnBTreeSplit(tree, newRoot, 0, &localResult);
		if (localResult != CMN_SUCCESS) {
			CMN_SET_RESULT(result, localResult);
			return;
		}

		cmnBTreeInsertNotNull(tree, newRoot, key, element, &localResult);
		if (localResult != CMN_SUCCESS) {
			CMN_SET_RESULT(result, localResult);
			return;
		}

		tree->root = newRoot;
	} else {
		cmnBTreeInsertNotNull(tree, tree->root, key, element, &localResult);
		if (localResult != CMN_SUCCESS) {
			CMN_SET_RESULT(result, localResult);
			return;
		}
	}

	CMN_SET_RESULT(result, CMN_SUCCESS);
}

template <typename K, typename V>
bool cmnContains(CmnBTree<K, V>* tree, const K& key) {
	size_t indexInNode;
	CmnBTreeNode<K, V>* node = cmnBTreeSearchNode(tree->root, key, &indexInNode);
	
	return node != nullptr;
}

template <typename K, typename V>
V& cmnGet(CmnBTree<K, V>* tree, const K& key, bool* didFindElement) {
	size_t indexInNode;
	CmnBTreeNode<K, V>* node = cmnBTreeSearchNode(tree->root, key, &indexInNode);
	
	if (node == nullptr) {
		if (didFindElement != nullptr) {
			*didFindElement = false;
		}
		return tree->defaultElement;
	} else {
		if (didFindElement != nullptr) {
			*didFindElement = true;
		}
		return node->elements[indexInNode];
	}
}

template <typename K, typename V>
void cmnRemove(CmnBTree<K, V>* tree, const K& key) {
	cmnBTreeRemove(tree, tree->root, key);

	if (tree->root->keyCount == 0 && !tree->root->isLeaf) {
		CmnBTreeNode<K, V>* oldRoot = tree->root;
		tree->root = tree->root->children[0];

		cmnPoolFree(tree->nodesPool, oldRoot);
	}
}

#endif // CMN_BTREE_H

