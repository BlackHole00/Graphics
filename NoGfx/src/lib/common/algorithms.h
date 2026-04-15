#ifndef CMN_ALGORITHMS_H
#define CMN_ALGORITHMS_H

#include <lib/common/common.h>
#include <lib/common/type_traits.h>

/**
	Inserts an element at the requested index, shifting subsequent elements to the right.

	@param elements The target array.
	@param elementCount The current element count.
	@param index The destination insertion index.
	@param element The element to insert.
*/
template <typename T>
void cmnInsertAtIndex(T* elements, size_t elementCount, size_t index, const T& element) {
	if (index > elementCount) {
		return;
	}

	for (size_t i = elementCount; i > index; i--) {
		elements[i] = elements[i - 1];
	}

	elements[index] = element;
}

/**
	Inserts an element preserving sorted order.

	@param elements The target sorted array.
	@param elementCount The current element count.
	@param element The element to insert.
*/
template <typename T>
void cmnOrderedInsert(T* elements, size_t elementCount, const T& element) {
	size_t i = 0;
	while (i < elementCount && cmnCmp(elements[i], element) == CMN_MORE) {
		i++;
	}

	cmnInsertAtIndex(elements, elementCount, i, element);
}

/**
	Removes the element at the requested index, shifting subsequent elements to the left.

	@param elements The target array.
	@param elementCount The current element count.
	@param index The index of the element to remove.
*/
template <typename T>
void cmnRemoveAtIndex(T* elements, size_t elementCount, size_t index) {
	size_t i = index;
	while (i < (elementCount - 1)) {
		elements[i] = elements[i + 1];
		i++;
	}
}

/**
	Removes an element from a sorted array.

	@param elements The target sorted array.
	@param elementCount The current element count.
	@param element The element to remove.
*/
template <typename T>
void cmnOrderedRemove(T* elements, size_t elementCount, const T& element) {
	size_t i = 0;
	while (i < elementCount && cmnCmp(elements[i], element) == CMN_LESS) {
		i++;
	}

	if (cmnCmp(elements[i], element) != CMN_EQUALS) {
		return;
	}

	cmnRemoveAtIndex(elements, elementCount, i);
}

/**
	Performs a linear search for an element.

	@param elements The target array.
	@param elementCount The current element count.
	@param element The element to search.
	@param[out] didFindElement Optional output flag indicating whether the element was found.

	@return The index of the found element, or `SIZE_MAX` when not found.
*/
template <typename T>
size_t cmnLinearSearch(T* elements, size_t elementCount, const T& element, bool* didFindElement = nullptr) {
	size_t i = 0;
	while (i < elementCount && !cmnEq(elements[i], element)) {
		i++;
	}

	if (i == elementCount) {
		if (didFindElement != nullptr) {
			*didFindElement = false;
		}

		return SIZE_MAX;
	} else {
		if (didFindElement != nullptr) {
			*didFindElement = true;
		}

		return i;
	}
}

/**
	Finds the first index whose element is greater than or equal to the requested element.

	@param elements The target sorted array.
	@param elementCount The current element count.
	@param element The element to compare.

	@return The first matching index, or `elementCount` when no index matches.
*/
template <typename T>
size_t cmnFindFirstGreaterElementIndex(T* elements, size_t elementCount, const T& element) {
	size_t i = 0;
	while (i < elementCount && cmnCmp(elements[i], element) == CMN_LESS) {
		i++;
	}

	return i;
}

#endif // CMN_ALGORITHMS_H

