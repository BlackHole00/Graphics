#include <stdio.h>

#include <stdlib.h>
#include <lib/common/exponential_array.h>

static_assert(sizeof(size_t) == 8, "");

int main(void) {
	uint8_t* memory = (uint8_t*)malloc(1024 * 10);

	CmnArena arena = cmnCreateArena(memory, 1024 * 10);

	CmnExponentialArray<int> arr;
	cmnCreateExponentialArray(&arr, &arena);

	for (int i = 0; i < 256; i++) {
		size_t a, b;
		cmnDecomposeExponentialArrayIndex(i, &a, &b);
		printf("%d\t%d\t%d\n", i, a, b);

		cmnAppend(&arr, i);
	}

	for (int i = 0; i < 256; i++) {
		assert(arr[i] == i);
	}

	arr[10] = 11;
	assert(arr[10] = 11);

	return 0;
}

