#ifndef GPU_POINTERRANGETREE_H
#define GPU_POINTERRANGETREE_H

#include <Metal/Metal.h>

#include <lib/common/arena.h>
#include <lib/metal4/context.h>

typedef struct Mtl4BufferHandle {
	int32_t	generation;
	int32_t	index;
} Mtl4BufferHandle;

typedef struct Mtl4BufferData {
	id<MTLBuffer>	buffer;
} Mtl4BufferData;

typedef struct Mtl4PointerMapNode {
	bool				isLeaf;

	struct Mtl4PointerMapNode*	left;
	uintptr_t			leftCompression;
	struct Mtl4PointerMapNode*	right;
	uintptr_t			rightCompression;

	// If invalid, then the corrisponding buffer was deallocated. We remove nodes with invalid handles on visits.
	Mtl4BufferHandle		handle;
} Mtl4PointerMapNode;

typedef struct Mtl4PointerMap {
	Mtl4PointerMapNode* root;
} Mtl4PointerMap;
extern Mtl4PointerMapNode gMtl4PointerMap;

inline void mtl4AddressSpaceToNodes(uintptr_t basePointer, size_t length, uintptr_t** nodes, size_t* nodesCount) {
	*nodes = cmnArenaAlloc<uintptr_t>(&gMtl4Context.tempArena, 64, nullptr);
	*nodesCount = 0;
	// check for failure

	uintptr_t start = basePointer;
	uintptr_t end = basePointer + length - 1;

	while (start <= end) {
		size_t size = 1;

		while (start % (2 * size) == 0) {
			size = size * 2;
		}

		while (start + size - 1 > end) {
			size = size / 2;
		}

		(*nodes)[*nodesCount] = start;
		*nodesCount += 1;

		start = start + size;
	}
}

inline void mtl4RegisterBufferAddressSpace(Mtl4BufferHandle buffer, uintptr_t basePointer, size_t length) {
	// CmnArenaTempGuard arenaTemp(&gMtl4Context.tempArena);
	
	uintptr_t* nodes;
	size_t size;
	mtl4AddressSpaceToNodes(basePointer, length, &nodes, &size);
	for (size_t i = 0; i < size; i++) {
		printf("N: %8lx\n", nodes[i]);
	}
}

inline void testPointerMap(void) {
	id<MTLBuffer> buffer = [gMtl4Context.device newBufferWithLength:768 options:MTLResourceStorageModeShared];

	uintptr_t basePointer = (uintptr_t)[buffer contents];
	size_t size = 768;
	printf("P: %8lx %lu\n", basePointer, size);
	mtl4RegisterBufferAddressSpace({}, basePointer, size);

	[buffer release];
}

#endif

