#ifndef CMN_MEMORY_H
#define CMN_MEMORY_H

#include <lib/common/common.h>

/**
	Checks whether `ptr` satisfies the requested byte alignment.

	@param ptr The address value to validate.
	@param alignment Required alignment in bytes.

	@return `true` if `ptr` is aligned to `alignment`, otherwise `false`.
*/
bool cmnIsAlignedTo(uintptr_t ptr, size_t alignment);

/**
	Rounds `ptr` up to the next address aligned to `alignment`.

	@param ptr The address value to round.
	@param alignment Required alignment in bytes.

	@return The smallest address `>= ptr` that is aligned to `alignment`.
*/
uintptr_t cmnUpAlignTo(uintptr_t ptr, size_t alignment);

/**
	Rounds `ptr` down to the previous address aligned to `alignment`.

	@param ptr The address value to round.
	@param alignment Required alignment in bytes.

	@return The largest address `<= ptr` that is aligned to `alignment`.
*/
uintptr_t cmnDownAlignTo(uintptr_t ptr, size_t alignment);

#endif // CMN_MEMORY_H

