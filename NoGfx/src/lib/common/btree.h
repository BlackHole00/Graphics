#ifndef CMN_BTREE_H
#define CMN_BTREE_H

#include <lib/common/common.h>
#include <lib/common/type_traits.h>
#include <lib/common/allocator.h>
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
	CmnAllocator	nodesAllocator;

	CmnBTreeNode<K, V>*	root;
	V			defaultElement;
};

template <typename K, typename V>
void cmnCreateBTree(CmnBTree<K, V>* tree, const V& defaultElement, CmnAllocator nodesAllocator, CmnResult* result);

template <typename K, typename V>
void cmnBTreeSplit(CmnBTree<K, V>* tree, CmnBTreeNode<K, V>* node, size_t index, CmnResult* result);

template <typename K, typename V>
void cmnBTreeInsertNotNull(
	CmnBTree<K, V>* tree,
	CmnBTreeNode<K, V>* node,
	const K& key,
	const V& element,
	CmnResult* result
);

template <typename K, typename V>
CmnBTreeNode<K, V>* cmnBTreeSearchNode(CmnBTreeNode<K, V>* node, const K& key, size_t* indexInNode);

template <typename K, typename V>
void cmnBTreePredecessor(CmnBTreeNode<K, V>* node, size_t index, K* key, V* element);

template <typename K, typename V>
void cmnBTreeSuccessor(CmnBTreeNode<K, V>* node, size_t index, K* key, V* element);

template <typename K, typename V>
void cmnBTreeBorrowFromPrev(CmnBTreeNode<K, V>* node, size_t index);

template <typename K, typename V>
void cmnBTreeBorrowFromNext(CmnBTreeNode<K, V>* node, size_t index);

template <typename K, typename V>
void cmnBTreeMerge(CmnBTree<K, V>* tree, CmnBTreeNode<K, V>* node, size_t index);

template <typename K, typename V>
void cmnBTreeFill(CmnBTree<K, V>* tree, CmnBTreeNode<K, V>* node, size_t index);

template <typename K, typename V>
void cmnBTreeRemove(CmnBTree<K, V>* tree, CmnBTreeNode<K, V>* node, const K& key);

template <typename K, typename V>
void cmnBTreeRemoveFromNonLeaf(CmnBTree<K, V>* tree, CmnBTreeNode<K, V>* node, size_t index);

template <typename K, typename V>
void cmnBTreeRemoveFromLeaf(CmnBTreeNode<K, V>* node, size_t index);

template <typename K, typename V>
void cmnInsert(CmnBTree<K, V>* tree, const K& key, const V& element, CmnResult* result);

template <typename K, typename V>
bool cmnContains(CmnBTree<K, V>* tree, const K& key);

template <typename K, typename V>
V& cmnGet(CmnBTree<K, V>* tree, const K& key, bool* didFindElement);

template <typename K, typename V>
void cmnRemove(CmnBTree<K, V>* tree, const K& key);

#include "btree.inc"

#endif // CMN_BTREE_H
