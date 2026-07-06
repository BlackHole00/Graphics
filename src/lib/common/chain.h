#ifndef CMN_CHAIN_H
#define CMN_CHAIN_H

#include <lib/common/common.h>
#include <lib/common/allocator.h>
#include <lib/common/rw_mutex.h>
#include <lib/common/type_traits.h>

template <typename T, size_t N = 15>
struct CmnChainNode {
	// Values stored in this node.
	T			elements[N];

	// Number of valid elements currently stored in `elements`.
	uint16_t		count;
	// Next node in the chain.
	CmnChainNode<T, N>*	next;
};

// Bucketed linked chain of values protected by an internal reader-writer mutex.
//
// The chain stores values in fixed-size nodes and grows by allocating new nodes on demand.
template <typename T, size_t N = 15>
struct CmnChain {
	// First node in the chain, or nullptr when empty.
	CmnChainNode<T, N>*	head;

	// Synchronizes chain operations.
	CmnRWMutex		mutex;
};

// Initializes an empty chain.
//
// Inputs:
// - chain: Chain to initialize.
// - result: Optional operation result.
//
// Results:
// - CMN_SUCCESS: Initialization succeeded.
template <typename T, size_t N = 15>
void cmnCreateChain(CmnChain<T, N>* chain);

// Releases all nodes owned by the chain.
//
// Inputs:
// - chain: Chain to destroy.
// - allocator: Allocator used to release node storage.
template <typename T, size_t N = 15>
void cmnDestroyChain(CmnChain<T, N>* chain, CmnAllocator allocator);

// Inserts a value into the chain.
//
// Inputs:
// - chain: Target chain.
// - value: Value to insert.
// - allocator: Allocator used when a new node is required.
// - result: Optional operation result.
//
// Results:
// - CMN_SUCCESS: Insert succeeded.
// - CMN_OUT_OF_MEMORY: Allocator ran out of memory while growing the chain.
template <typename T, size_t N = 15>
void cmnInsert(CmnChain<T, N>* chain, const T& value, CmnAllocator allocator, CmnResult* result);

// Removes the first matching value from the chain, if present.
//
// Inputs:
// - chain: Target chain.
// - value: Value to remove.
// - allocator: Allocator used when releasing empty nodes.
template <typename T, size_t N = 15>
void cmnRemove(CmnChain<T, N>* chain, const T& value, CmnAllocator allocator);

// Checks whether the chain contains a value.
//
// Inputs:
// - chain: Target chain.
// - value: Value to search for.
//
// Returns:
// - true when a matching value exists.
template <typename T, size_t N = 15>
bool cmnContains(CmnChain<T, N>* chain, const T& value);

// Forward iterator over a chain.
template <typename T, size_t N = 15>
struct CmnChainIterator {
	// Chain being traversed.
	CmnChain<T, N>*		chain;
	// Current node in the traversal, or nullptr when finished.
	CmnChainNode<T, N>*	currentNode;
	// Index of the next element to return from `currentNode`.
	uint16_t		currentElementInNode;
};

// Initializes an iterator at the beginning of a chain.
//
// Inputs:
// - chain: Chain to iterate.
// - iter: Iterator to initialize.
//
// Remarks:
// - The caller must hold a read lock on `chain->mutex` for the entire traversal
template <typename T, size_t N = 15>
void cmnCreateChainIterator(CmnChain<T, N>* chain, CmnChainIterator<T, N>* iter);

// Gets the next element of a chain
//
// Inputs:
// - iter: The iterator.
// - value: Output current element.
//
// Returns:
// - True if the current element is value. False otherwise.
template <typename T, size_t N = 15>
bool cmnIterate(CmnChainIterator<T, N>* iter, T** value);

#include "chain.inc"

#endif // CMN_CHAIN_H

