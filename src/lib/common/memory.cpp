#include "memory.h"

bool cmnIsAlignedTo(uintptr_t ptr, size_t alignment) {
	if (alignment == 0) {
		alignment = 1;
	}

	return (ptr % alignment) == 0;
}

uintptr_t cmnUpAlignTo(uintptr_t ptr, size_t alignment) {
	if (alignment == 0) {
		alignment = 1;
	}

	uintptr_t remainder = ptr % alignment;
	if (remainder == 0) {
		return ptr;
	}

	return ptr + (alignment - remainder);
}

uintptr_t cmnDownAlignTo(uintptr_t ptr, size_t alignment) {
	if (alignment == 0) {
		alignment = 1;
	}

	uintptr_t remainder = ptr % alignment;
	if (remainder == 0) {
		return ptr;
	}

	return ptr - remainder;
}
