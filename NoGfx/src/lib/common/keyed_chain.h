#ifndef CMN_KEYEDCHAIN_H
#define CMN_KEYEDCHAIN_H

#include <lib/common/common.h>
#include <lib/common/allocator.h>
#include <lib/common/rw_mutex.h>
#include <lib/common/type_traits.h>

template <typename K, typename V, size_t N = 15>
struct CmnKeyedChainNode {
	// Keys stored in this node.
	K		keys[N];
	// Values associated with the keys.
	V		values[N];

	// Number of valid entries currently stored in this node.
	uint16_t	count;
	// Next node in the chain.
	CmnKeyedChainNode<K, V, N>*	next;
};

// Bucketed linked chain of key-value pairs protected by an internal reader-writer mutex.
//
// The chain stores entries in fixed-size nodes and grows by allocating new nodes on demand.
template <typename K, typename V, size_t N = 15>
struct CmnKeyedChain {
	// Value returned by `cmnGet` when a key is missing.
	V			defaultValue;

	// First node in the chain, or nullptr when empty.
	CmnKeyedChainNode<K, V, N>*	head;

	// Synchronizes chain operations.
	CmnRWMutex		mutex;
};

// Initializes an empty keyed chain.
//
// Inputs:
// - chain: Chain to initialize.
// - defaultValue: Value returned by failed lookups.
//
// Results:
// - CMN_SUCCESS: Initialization succeeded.
template <typename K, typename V, size_t N = 15>
void cmnCreateKeyedChain(CmnKeyedChain<K, V, N>* chain, const V& defaultValue);

// Releases all nodes owned by the keyed chain.
//
// Inputs:
// - chain: Chain to destroy.
// - allocator: Allocator used to release node storage.
template <typename K, typename V, size_t N = 15>
void cmnDestroyKeyedChain(CmnKeyedChain<K, V, N>* chain, CmnAllocator allocator);

// Inserts or overwrites a key-value pair.
//
// Inputs:
// - chain: Target chain.
// - key: Key to insert or update.
// - value: Value to associate with `key`.
// - allocator: Allocator used when a new node is required.
// - result: Optional operation result.
//
// Results:
// - CMN_SUCCESS: Insert succeeded.
// - CMN_OUT_OF_MEMORY: Allocator ran out of memory while growing the chain.
template <typename K, typename V, size_t N = 15>
void cmnInsert(CmnKeyedChain<K, V, N>* chain, const K& key, const V& value, CmnAllocator allocator, CmnResult* result);

// Removes the first matching key from the chain, if present.
//
// Inputs:
// - chain: Target chain.
// - key: Key to remove.
// - allocator: Allocator used when releasing empty nodes.
template <typename K, typename V, size_t N = 15>
void cmnRemove(CmnKeyedChain<K, V, N>* chain, const K& key, CmnAllocator allocator);

// Checks whether a key exists in the chain.
//
// Inputs:
// - chain: Target chain.
// - key: Key to search for.
//
// Returns:
// - true when a matching key exists.
template <typename K, typename V, size_t N = 15>
bool cmnContains(CmnKeyedChain<K, V, N>* chain, const K& key);

// Returns the value associated with a key.
//
// Inputs:
// - chain: Target chain.
// - key: Key to search for.
// - found: Optional output flag.
//
// Returns:
// - Matching value reference, or `defaultValue` when missing.
template <typename K, typename V, size_t N = 15>
V& cmnGet(CmnKeyedChain<K, V, N>* chain, const K& key, bool* found);

// Forward iterator over a keyed chain.
template <typename K, typename V, size_t N = 15>
struct CmnKeyedChainIterator {
	// Chain being traversed.
	CmnKeyedChain<K, V, N>*		chain;
	// Current node in the traversal, or nullptr when finished.
	CmnKeyedChainNode<K, V, N>*	currentNode;
	// Index of the next entry to return from `currentNode`.
	uint16_t			currentElementInNode;
};

// Initializes an iterator at the beginning of a keyed chain.
//
// Inputs:
// - chain: Chain to iterate.
// - iter: Iterator to initialize.
//
// Remarks:
// - The caller must hold a read lock on `chain->mutex` for the entire traversal.
template <typename K, typename V, size_t N = 15>
void cmnCreateKeyedChainIterator(CmnKeyedChain<K, V, N>* chain, CmnKeyedChainIterator<K, V, N>* iter);

// Gets the next key-value pair of a keyed chain.
//
// Inputs:
// - iter: The iterator.
// - key: Output current key.
// - value: Output current value.
//
// Returns:
// - True if a current element was produced. False otherwise.
template <typename K, typename V, size_t N = 15>
bool cmnIterate(CmnKeyedChainIterator<K, V, N>* iter, K** key, V** value);

#include "keyed_chain.inc"

#endif // CMN_KEYEDCHAIN_H