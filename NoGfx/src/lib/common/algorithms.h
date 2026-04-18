#ifndef CMN_ALGORITHMS_H
#define CMN_ALGORITHMS_H

#include <lib/common/common.h>
#include <lib/common/type_traits.h>

// cmnInsertAtIndex inserts element at index and shifts later elements right.
//
// Inputs:
// - elements: Destination array buffer.
// - elementCount: Number of valid elements currently stored.
// - index: Insertion index in [0, elementCount].
// - element: Value to insert.
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

// cmnOrderedInsert inserts element into a sorted array while preserving order.
//
// Inputs:
// - elements: Sorted destination array buffer.
// - elementCount: Number of valid elements currently stored.
// - element: Value to insert.
template <typename T>
void cmnOrderedInsert(T* elements, size_t elementCount, const T& element) {
	size_t i = 0;
	while (i < elementCount && cmnCmp(elements[i], element) == CMN_MORE) {
		i++;
	}

	cmnInsertAtIndex(elements, elementCount, i, element);
}

// cmnRemoveAtIndex removes the element at index and shifts later elements left.
//
// Inputs:
// - elements: Destination array buffer.
// - elementCount: Number of valid elements currently stored.
// - index: Index to remove.
template <typename T>
void cmnRemoveAtIndex(T* elements, size_t elementCount, size_t index) {
	size_t i = index;
	while (i < (elementCount - 1)) {
		elements[i] = elements[i + 1];
		i++;
	}
}

// cmnOrderedRemove removes element from a sorted array if present.
//
// Inputs:
// - elements: Sorted destination array buffer.
// - elementCount: Number of valid elements currently stored.
// - element: Value to remove.
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

// cmnLinearSearch scans linearly for element.
//
// Inputs:
// - elements: Array buffer to search.
// - elementCount: Number of valid elements.
// - element: Value to find.
// - didFindElement: Optional output flag.
//
// Returns:
// - Index of the matching element, or SIZE_MAX when not found.
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

// cmnFindFirstGreaterElementIndex finds the first index with value >= element.
//
// Inputs:
// - elements: Sorted array buffer.
// - elementCount: Number of valid elements.
// - element: Value to compare against.
//
// Returns:
// - First index whose value is >= element, or elementCount.
template <typename T>
size_t cmnFindFirstGreaterElementIndex(T* elements, size_t elementCount, const T& element) {
	size_t i = 0;
	while (i < elementCount && cmnCmp(elements[i], element) == CMN_LESS) {
		i++;
	}

	return i;
}

#endif // CMN_ALGORITHMS_H

