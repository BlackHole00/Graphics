#include "test.h"

#include <stdio.h>
#include <stdlib.h>

TestingContext gTestingContext;

bool doTests(TestRecord* tests, size_t testCount) {
	gTestingContext.testCount = testCount;

	gTestingContext.results = (Test*)malloc(sizeof(Test) * testCount);
	for (size_t i = 0; i < testCount; i++) {
		gTestingContext.results[i].status = TEST_NOT_YET_RUN;
		gTestingContext.results[i].record = &tests[i];
	}

	bool didFail = false;
	for (size_t i = 0; i < testCount; i++) {
		Test* test = &gTestingContext.results[i];

		printf("[%llu] Running test `%s`:\n", (unsigned long long)i, test->record->name);

		test->status = TEST_RUNNING;
		if (setjmp(gTestingContext.jumpBuffer) == 0) {
			test->record->proc(test);
		}

		if (test->status == TEST_RUNNING) {
			test->status = TEST_SUCCESSFULL;
			printf("\tResult: PASS.\n");
		} else if (test->status == TEST_OUT_OF_MEMORY) {
			printf("\tResult: OUT OF MEMORY\n");
		} else {
			printf("\tResult: FAIL.\n");
			didFail = true;
		}
	}

	if (didFail) {
		printf("Testsuite: FAIL.\n");
	} else {
		printf("Testsuite: PASS.\n");
	}

	return didFail;
}

void testAssertRaw(
	Test* test,
	bool condition,
	const char* expression,
	const char* file,
	int line,
	const char* function
) {
	if (condition) {
		return;
	}

	test->status = TEST_FAILED;
	printf("\t(%s - %s::%d) Failed assertion on expression `%s`.\n", function, file, line, expression);

	longjmp(gTestingContext.jumpBuffer, 1);
}
void testPanicRaw(Test* test, const char* message, const char* file, int line, const char* function) {
	test->status = TEST_FAILED;
	printf("\t(%s - %s::%d) Panicked with message `%s`.\n", function, file, line, message);

	longjmp(gTestingContext.jumpBuffer, 1);
}

void testOutOfMemory(Test* test) {
	test->status = TEST_OUT_OF_MEMORY;
	printf("\t Test run out of memory.\n");

	longjmp(gTestingContext.jumpBuffer, 1);
}

