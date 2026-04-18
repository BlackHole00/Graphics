#ifndef CMN_BTREE_H
#define CMN_BTREE_H

#include <lib/common/common.h>
#include <lib/common/type_traits.h>
#include <lib/common/allocator.h>
#include <lib/common/algorithms.h>

// Maximum number of children in each B-tree node.
#define CMN_BTREE_ORDER	16
// Minimum degree of the B-tree.
#define CMN_BTREE_T	8

// Single node of a B-tree.
template <typename K, typename V>
struct CmnBTreeNode {
	bool		isLeaf;
	uint8_t		keyCount;

	K			keys	[CMN_BTREE_ORDER - 1];
	V			elements[CMN_BTREE_ORDER - 1];
	CmnBTreeNode<K, V>*	children[CMN_BTREE_ORDER];
};

// B-tree container
template <typename K, typename V>
struct CmnBTree {
	CmnAllocator	nodesAllocator;

	CmnBTreeNode<K, V>*	root;
	V			defaultElement;
};

// Initializes a B-tree.
//
// Inputs:
// - tree: Tree to initialize.
// - defaultElement: Value returned when lookups fail.
// - nodesAllocator: Allocator used for nodes.
// - result: Optional operation result.
//
// Results:
// - CMN_SUCCESS: Initialization succeeded.
// - CMN_OUT_OF_MEMORY: Backing allocator ran out of memory.
template <typename K, typename V>
void cmnCreateBTree(CmnBTree<K, V>* tree, const V& defaultElement, CmnAllocator nodesAllocator, CmnResult* result);

// Splits a full child node during insertion.
//
// Inputs:
// - tree: Target tree.
// - node: Parent node.
// - index: Child index to split.
// - result: Optional operation result.
//
// Results:
// - CMN_SUCCESS: Split succeeded.
// - CMN_OUT_OF_MEMORY: Backing allocator ran out of memory.
template <typename K, typename V>
void cmnBTreeSplit(CmnBTree<K, V>* tree, CmnBTreeNode<K, V>* node, size_t index, CmnResult* result);

// Inserts into a subtree rooted at node.
//
// Inputs:
// - tree: Target tree.
// - node: Current node.
// - key: Key to insert.
// - element: Value to insert.
// - result: Optional operation result.
//
// Results:
// - CMN_SUCCESS: Insert step succeeded.
// - CMN_OUT_OF_MEMORY: Backing allocator ran out of memory.
template <typename K, typename V>
void cmnBTreeInsertNotNull(
	CmnBTree<K, V>* tree,
	CmnBTreeNode<K, V>* node,
	const K& key,
	const V& element,
	CmnResult* result
);

// Searches for a key starting from node.
//
// Inputs:
// - node: Search start node.
// - key: Key to find.
// - indexInNode: Output index in found node.
//
// Returns:
// - Pointer to node containing key, or nullptr if missing.
template <typename K, typename V>
CmnBTreeNode<K, V>* cmnBTreeSearchNode(CmnBTreeNode<K, V>* node, const K& key, size_t* indexInNode);

// Gets predecessor key/value for node entry.
//
// Inputs:
// - node: Node containing reference entry.
// - index: Entry index.
// - key: Output predecessor key.
// - element: Output predecessor value.
template <typename K, typename V>
void cmnBTreePredecessor(CmnBTreeNode<K, V>* node, size_t index, K* key, V* element);

// Gets successor key/value for node entry.
//
// Inputs:
// - node: Node containing reference entry.
// - index: Entry index.
// - key: Output successor key.
// - element: Output successor value.
template <typename K, typename V>
void cmnBTreeSuccessor(CmnBTreeNode<K, V>* node, size_t index, K* key, V* element);

// Borrows one key from the previous sibling.
//
// Inputs:
// - node: Parent node.
// - index: Borrowing child index.
template <typename K, typename V>
void cmnBTreeBorrowFromPrev(CmnBTreeNode<K, V>* node, size_t index);

// Borrows one key from the next sibling.
//
// Inputs:
// - node: Parent node.
// - index: Borrowing child index.
template <typename K, typename V>
void cmnBTreeBorrowFromNext(CmnBTreeNode<K, V>* node, size_t index);

// Merges a child with its next sibling.
//
// Inputs:
// - tree: Target tree.
// - node: Parent node.
// - index: Child index to merge.
template <typename K, typename V>
void cmnBTreeMerge(CmnBTree<K, V>* tree, CmnBTreeNode<K, V>* node, size_t index);

// Ensures a child has enough keys before descending.
//
// Inputs:
// - tree: Target tree.
// - node: Parent node.
// - index: Child index to fix.
template <typename K, typename V>
void cmnBTreeFill(CmnBTree<K, V>* tree, CmnBTreeNode<K, V>* node, size_t index);

// Removes key from subtree rooted at node.
//
// Inputs:
// - tree: Target tree.
// - node: Subtree root node.
// - key: Key to remove.
template <typename K, typename V>
void cmnBTreeRemove(CmnBTree<K, V>* tree, CmnBTreeNode<K, V>* node, const K& key);

// Removes key from an internal node entry.
//
// Inputs:
// - tree: Target tree.
// - node: Node containing key.
// - index: Entry index to remove.
template <typename K, typename V>
void cmnBTreeRemoveFromNonLeaf(CmnBTree<K, V>* tree, CmnBTreeNode<K, V>* node, size_t index);

// Removes key from a leaf node entry.
//
// Inputs:
// - node: Leaf node containing key.
// - index: Entry index to remove.
template <typename K, typename V>
void cmnBTreeRemoveFromLeaf(CmnBTreeNode<K, V>* node, size_t index);

// Inserts a key-value pair into the tree.
//
// Inputs:
// - tree: Target tree.
// - key: Key to insert.
// - element: Value to insert.
// - result: Optional operation result.
//
// Results:
// - CMN_SUCCESS: Insert succeeded.
// - CMN_OUT_OF_MEMORY: Backing allocator ran out of memory.
template <typename K, typename V>
void cmnInsert(CmnBTree<K, V>* tree, const K& key, const V& element, CmnResult* result);

// Checks whether key exists in the tree.
//
// Inputs:
// - tree: Target tree.
// - key: Key to search.
//
// Returns:
// - true when key exists.
template <typename K, typename V>
bool cmnContains(CmnBTree<K, V>* tree, const K& key);

// Gets the value associated with key.
//
// Inputs:
// - tree: Target tree.
// - key: Key to search.
// - didFindElement: Optional output flag.
//
// Returns:
// - Found value reference, or defaultElement when missing.
template <typename K, typename V>
V& cmnGet(CmnBTree<K, V>* tree, const K& key, bool* didFindElement);

// Removes key from the tree.
//
// Inputs:
// - tree: Target tree.
// - key: Key to remove.
template <typename K, typename V>
void cmnRemove(CmnBTree<K, V>* tree, const K& key);

#include "btree.inc"

#endif // CMN_BTREE_H
