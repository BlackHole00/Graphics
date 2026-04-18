#ifndef CMN_MEMORY_H
#define CMN_MEMORY_H

#include <lib/common/common.h>

// Reports whether ptr satisfies the requested alignment.
//
// Inputs:
// - ptr: Address value to test.
// - alignment: Required byte alignment.
//
// Returns:
// - true when ptr is aligned to alignment.
bool cmnIsAlignedTo(uintptr_t ptr, size_t alignment);

// Rounds ptr up to the next aligned address.
//
// Inputs:
// - ptr: Address value to round.
// - alignment: Required byte alignment.
//
// Returns:
// - Smallest address >= ptr aligned to alignment.
uintptr_t cmnUpAlignTo(uintptr_t ptr, size_t alignment);

// Rounds ptr down to the previous aligned address.
//
// Inputs:
// - ptr: Address value to round.
// - alignment: Required byte alignment.
//
// Returns:
// - Largest address <= ptr aligned to alignment.
uintptr_t cmnDownAlignTo(uintptr_t ptr, size_t alignment);

#endif // CMN_MEMORY_H

