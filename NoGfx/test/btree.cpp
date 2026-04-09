#include "test.h"

#include <stdlib.h>
#include <lib/common/common.h>
#include <lib/common/btree.h>

void checkBTreeCreation(Test* test) {
	uint8_t* backingMemory = (uint8_t*)malloc(1024 * sizeof(uint8_t));
	if (backingMemory == nullptr) {
		testOutOfMemory(test);
	}

	CmnPool pool = cmnCreatePool(backingMemory, 1024, sizeof(CmnBTreeNode<int32_t, int32_t>), 16);

	CmnBTree<int32_t, int32_t> tree;
	cmnCreateBTree(&tree, 0, &pool, nullptr);

	TEST_ASSERT(test, tree.root != nullptr);
	TEST_ASSERT(test, tree.root->keyCount == 0);
	TEST_ASSERT(test, tree.root->isLeaf == true);
}

void checkBTreeInsertAndContains(Test* test) {
	uint8_t* backingMemory = (uint8_t*)malloc(2048);
	if (backingMemory == nullptr) {
		testOutOfMemory(test);
	}

	CmnPool pool = cmnCreatePool(backingMemory, 2048, sizeof(CmnBTreeNode<int32_t, int32_t>), 16);
	CmnBTree<int32_t, int32_t> tree;
	cmnCreateBTree(&tree, -1, &pool, nullptr);

	cmnInsert(&tree, 10, 100, nullptr);
	cmnInsert(&tree, 20, 200, nullptr);
	cmnInsert(&tree, 5, 50, nullptr);

	TEST_ASSERT(test, cmnContains(&tree, 10));
	TEST_ASSERT(test, cmnContains(&tree, 20));
	TEST_ASSERT(test, cmnContains(&tree, 5));
	TEST_ASSERT(test, !cmnContains(&tree, 15));
}

void checkBTreeGet(Test* test) {
	uint8_t* backingMemory = (uint8_t*)malloc(2048);
	if (backingMemory == nullptr) {
		testOutOfMemory(test);
	}

	CmnPool pool = cmnCreatePool(backingMemory, 2048, sizeof(CmnBTreeNode<int32_t, int32_t>), 16);
	CmnBTree<int32_t, int32_t> tree;
	cmnCreateBTree(&tree, -1, &pool, nullptr);

	cmnInsert(&tree, 1, 10, nullptr);
	cmnInsert(&tree, 2, 20, nullptr);

	bool found = false;
	int32_t value = cmnGet(&tree, 2, &found);
	TEST_ASSERT(test, found && value == 20);

	value = cmnGet(&tree, 3, &found);
	TEST_ASSERT(test, !found && value == tree.defaultElement);
}

void checkBTreeRemoveLeaf(Test* test) {
	uint8_t* backingMemory = (uint8_t*)malloc(2048);
	if (backingMemory == nullptr) {
		testOutOfMemory(test);
	}

	CmnPool pool = cmnCreatePool(backingMemory, 2048, sizeof(CmnBTreeNode<int32_t, int32_t>), 16);
	CmnBTree<int32_t, int32_t> tree;
	cmnCreateBTree(&tree, -1, &pool, nullptr);

	cmnInsert(&tree, 1, 10, nullptr);
	cmnInsert(&tree, 2, 20, nullptr);
	cmnInsert(&tree, 3, 30, nullptr);

	cmnRemove(&tree, 2);
	TEST_ASSERT(test, !cmnContains(&tree, 2));
	TEST_ASSERT(test, cmnContains(&tree, 1));
	TEST_ASSERT(test, cmnContains(&tree, 3));
}

void checkBTreeRemoveNonLeaf(Test* test) {
	uint8_t* backingMemory = (uint8_t*)malloc(4096);
	if (backingMemory == nullptr) {
		testOutOfMemory(test);
	}

	CmnPool pool = cmnCreatePool(backingMemory, 4096, sizeof(CmnBTreeNode<int32_t, int32_t>), 16);
	CmnBTree<int32_t, int32_t> tree;
	cmnCreateBTree(&tree, -1, &pool, nullptr);

	// Insert enough keys to force a split
	for (int i = 1; i <= 20; i++) {
		cmnInsert(&tree, i, i * 10, nullptr);
	}

	cmnRemove(&tree, 10); // Remove a key from internal node
	TEST_ASSERT(test, !cmnContains(&tree, 10));

	for (int i = 1; i <= 20; i++) {
		if (i != 10) {
			TEST_ASSERT(test, cmnContains(&tree, i));
		}
	}
}

void checkBTreeRootSplit(Test* test) {
	uint8_t* backingMemory = (uint8_t*)malloc(8192);
	if (backingMemory == nullptr) {
		testOutOfMemory(test);
	}

	CmnPool pool = cmnCreatePool(backingMemory, 8192, sizeof(CmnBTreeNode<int32_t, int32_t>), 16);
	CmnBTree<int32_t, int32_t> tree;
	cmnCreateBTree(&tree, -1, &pool, nullptr);

	// Insert enough keys to force multiple splits
	for (int i = 1; i <= 50; i++) {
		cmnInsert(&tree, i, i * 100, nullptr);
	}

	TEST_ASSERT(test, tree.root->keyCount > 0);
	TEST_ASSERT(test, !tree.root->isLeaf);
}

void checkBTreePredecessorSuccessor(Test* test) {
	uint8_t* backingMemory = (uint8_t*)malloc(4096);
	if (backingMemory == nullptr) {
		testOutOfMemory(test);
	}

	CmnPool pool = cmnCreatePool(backingMemory, 4096, sizeof(CmnBTreeNode<int32_t, int32_t>), 16);
	CmnBTree<int32_t, int32_t> tree;
	cmnCreateBTree(&tree, -1, &pool, nullptr);

	for (int i = 1; i <= 20; i++) {
		cmnInsert(&tree, i, i * 10, nullptr);
	}

	int32_t predKey; int32_t predValue;
	int32_t succKey; int32_t succValue;

	cmnBTreePredecessor(tree.root, 0, &predKey, &predValue);
	cmnBTreeSuccessor(tree.root, 0, &succKey, &succValue);

	TEST_ASSERT(test, predKey < tree.root->keys[0]);
	TEST_ASSERT(test, succKey > tree.root->keys[0]);
}
