#ifndef CMN_BTREE_H
#define CMN_BTREE_H

#include <lib/common/common.h>
#include <lib/common/type_traits.h>
#include <lib/common/allocator.h>
#include <lib/common/algorithms.h>

/** The maximum number of children in each B-tree node. */
#define CMN_BTREE_ORDER	16
/** The minimum degree of the B-tree. */
#define CMN_BTREE_T	8

/**
	A single node of a B-tree.
	@see CmnBTree
*/
template <typename K, typename V>
struct CmnBTreeNode {
	/** True when this node is a leaf node. */
	bool		isLeaf;
	/** Number of valid keys in the node. */
	uint8_t		keyCount;

	/** Ordered keys stored in the node. */
	K			keys	[CMN_BTREE_ORDER - 1];
	/** Values associated with each key. */
	V			elements[CMN_BTREE_ORDER - 1];
	/** Child pointers for internal nodes. */
	CmnBTreeNode<K, V>*	children[CMN_BTREE_ORDER];
};

/**
	B-tree container mapping keys to values.
*/
template <typename K, typename V>
struct CmnBTree {
	/** Allocator used for node allocations. */
	CmnAllocator	nodesAllocator;

	/** Root node of the tree. */
	CmnBTreeNode<K, V>*	root;
	/** Value returned when lookups fail. */
	V			defaultElement;
};

/**
	Initializes a B-tree.

	@param tree The tree to initialize.
	@param defaultElement Value returned when lookups fail.
	@param nodesAllocator Allocator used for node storage.
	@param[out] result The result of the operation.
	@retval CMN_SUCCESS Tree initialization completed successfully.
	@retval CMN_OUT_OF_MEMORY Node allocation failed.
	@relates CmnBTree
*/
template <typename K, typename V>
void cmnCreateBTree(CmnBTree<K, V>* tree, const V& defaultElement, CmnAllocator nodesAllocator, CmnResult* result);

/**
	Splits a full child node during insertion.

	@param tree The target tree.
	@param node The parent node.
	@param index The child index to split.
	@param[out] result The result of the operation.
	@retval CMN_SUCCESS Node split completed successfully.
	@retval CMN_OUT_OF_MEMORY Node allocation failed.
	@relates CmnBTree
*/
template <typename K, typename V>
void cmnBTreeSplit(CmnBTree<K, V>* tree, CmnBTreeNode<K, V>* node, size_t index, CmnResult* result);

/**
	Inserts into a subtree rooted at a non-null node.

	@param tree The target tree.
	@param node The current node.
	@param key The key to insert.
	@param element The value to insert.
	@param[out] result The result of the operation.
	@retval CMN_SUCCESS Insertion into the subtree completed successfully.
	@retval CMN_OUT_OF_MEMORY Node allocation failed while splitting full nodes.
	@relates CmnBTree
*/
template <typename K, typename V>
void cmnBTreeInsertNotNull(
	CmnBTree<K, V>* tree,
	CmnBTreeNode<K, V>* node,
	const K& key,
	const V& element,
	CmnResult* result
);

/**
	Searches for a key starting from a node.

	@param node The node where search starts.
	@param key The key to find.
	@param[out] indexInNode Output index in the found node.

	@return Pointer to the node containing the key, or `nullptr` if not found.
*/
template <typename K, typename V>
CmnBTreeNode<K, V>* cmnBTreeSearchNode(CmnBTreeNode<K, V>* node, const K& key, size_t* indexInNode);

/**
	Gets predecessor key and value of a node entry.

	@param node The node containing the reference entry.
	@param index The entry index.
	@param[out] key Output predecessor key.
	@param[out] element Output predecessor value.
*/
template <typename K, typename V>
void cmnBTreePredecessor(CmnBTreeNode<K, V>* node, size_t index, K* key, V* element);

/**
	Gets successor key and value of a node entry.

	@param node The node containing the reference entry.
	@param index The entry index.
	@param[out] key Output successor key.
	@param[out] element Output successor value.
*/
template <typename K, typename V>
void cmnBTreeSuccessor(CmnBTreeNode<K, V>* node, size_t index, K* key, V* element);

/**
	Borrows one key from the previous sibling.

	@param node Parent node.
	@param index Child index that borrows.
*/
template <typename K, typename V>
void cmnBTreeBorrowFromPrev(CmnBTreeNode<K, V>* node, size_t index);

/**
	Borrows one key from the next sibling.

	@param node Parent node.
	@param index Child index that borrows.
*/
template <typename K, typename V>
void cmnBTreeBorrowFromNext(CmnBTreeNode<K, V>* node, size_t index);

/**
	Merges a child with its next sibling.

	@param tree The target tree.
	@param node Parent node.
	@param index Child index to merge.
*/
template <typename K, typename V>
void cmnBTreeMerge(CmnBTree<K, V>* tree, CmnBTreeNode<K, V>* node, size_t index);

/**
	Ensures a child has enough keys before descending.

	@param tree The target tree.
	@param node Parent node.
	@param index Child index to fix.
*/
template <typename K, typename V>
void cmnBTreeFill(CmnBTree<K, V>* tree, CmnBTreeNode<K, V>* node, size_t index);

/**
	Removes a key from a subtree.

	@param tree The target tree.
	@param node Subtree root node.
	@param key The key to remove.
*/
template <typename K, typename V>
void cmnBTreeRemove(CmnBTree<K, V>* tree, CmnBTreeNode<K, V>* node, const K& key);

/**
	Removes a key from an internal node.

	@param tree The target tree.
	@param node Node containing the key.
	@param index Entry index to remove.
*/
template <typename K, typename V>
void cmnBTreeRemoveFromNonLeaf(CmnBTree<K, V>* tree, CmnBTreeNode<K, V>* node, size_t index);

/**
	Removes a key from a leaf node.

	@param node Leaf node containing the key.
	@param index Entry index to remove.
*/
template <typename K, typename V>
void cmnBTreeRemoveFromLeaf(CmnBTreeNode<K, V>* node, size_t index);

/**
	Inserts a key-value pair into the tree.

	@param tree The target tree.
	@param key The key to insert.
	@param element The value to insert.
	@param[out] result The result of the operation.
	@retval CMN_SUCCESS Key-value insertion completed successfully.
	@retval CMN_OUT_OF_MEMORY Node allocation failed while splitting full nodes.
	@relates CmnBTree
*/
template <typename K, typename V>
void cmnInsert(CmnBTree<K, V>* tree, const K& key, const V& element, CmnResult* result);

/**
	Checks whether a key exists in the tree.

	@param tree The target tree.
	@param key The key to search.

	@return True if the key exists, false otherwise.
	@relates CmnBTree
*/
template <typename K, typename V>
bool cmnContains(CmnBTree<K, V>* tree, const K& key);

/**
	Gets the value associated with a key.

	@param tree The target tree.
	@param key The key to search.
	@param[out] didFindElement Output flag indicating whether the key exists.

	@return Reference to the found value, or to `defaultElement` when missing.
	@relates CmnBTree
*/
template <typename K, typename V>
V& cmnGet(CmnBTree<K, V>* tree, const K& key, bool* didFindElement);

/**
	Removes a key from the tree.

	@param tree The target tree.
	@param key The key to remove.
	@relates CmnBTree
*/
template <typename K, typename V>
void cmnRemove(CmnBTree<K, V>* tree, const K& key);

#include "btree.inc"

#endif // CMN_BTREE_H
