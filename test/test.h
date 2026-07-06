#ifndef TST_TEST_H
#define TST_TEST_H

#include <stddef.h>
#include <stdint.h>
#include <setjmp.h>

typedef enum TestStatus {
	TEST_NOT_YET_RUN,
	TEST_RUNNING,
	TEST_SUCCESSFULL,
	TEST_FAILED,
	TEST_OUT_OF_MEMORY,
} TestStatus;

typedef struct Test {
	struct TestRecord*	record;
	TestStatus		status;
} Test;

typedef void (*TestProc)(Test* test);

typedef struct TestRecord {
	const char*	name;
	TestProc	proc;
} TestRecord;

typedef struct TestingContext {
	Test*	results;
	size_t	testCount;

	jmp_buf	jumpBuffer;
} TestingContext;
extern TestingContext gTestingContext;

bool doTests(const char* suiteName, TestRecord* tests, size_t testCount);

#define TEST_STRINGIFY(...) #__VA_ARGS__
#define TEST_ASSERT(_test_ptr, ...) testAssertRaw(_test_ptr, __VA_ARGS__, TEST_STRINGIFY(__VA_ARGS__), __FILE__, __LINE__, __FUNCTION__)
#define TEST_PANIC(_test_ptr, _message) testPanicRaw(_test_ptr, _message, __FILE__, __LINE__, __FUNCTION__)

void testAssertRaw(Test* test, bool condition, const char* expression, const char* file, int line, const char* function);
void testPanicRaw(Test* test, const char* message, const char* file, int line, const char* function);
void testOutOfMemory(Test* test);

#endif

