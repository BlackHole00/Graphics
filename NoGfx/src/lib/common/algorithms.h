#ifndef CMN_ALGORITHMS_H
#define CMN_ALGORITHMS_H

#include <lib/common/common.h>
#include <lib/common/type_traits.h>

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

template <typename T>
void cmnOrderedInsert(T* elements, size_t elementCount, const T& element) {
	size_t i = 0;
	while (i < elementCount && cmnCmp(elements[i], element) == CMN_MORE) {
		i++;
	}

	cmnInsertAtIndex(elements, elementCount, i, element);
}

template <typename T>
void cmnRemoveAtIndex(T* elements, size_t elementCount, size_t index) {
	size_t i = index;
	while (i < (elementCount - 1)) {
		elements[i] = elements[i + 1];
		i++;
	}
}

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

template <typename T>
size_t cmnFindFirstGreaterElementIndex(T* elements, size_t elementCount, const T& element) {
	size_t i = 0;
	while (i < elementCount && cmnCmp(elements[i], element) == CMN_LESS) {
		i++;
	}

	return i;
}

#endif // CMN_ALGORITHMS_H

