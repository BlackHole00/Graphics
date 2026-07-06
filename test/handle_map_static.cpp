#include "test.h"

#include <lib/common/static_handle_map.h>

void checkForStaticHandleMapDataCoherency(Test* test) {
    CmnResult result;

    CmnStaticHandleMap<int32_t, 128> map;
    cmnCreateStaticHandleMap(&map, 0);

    CmnHandle handles[128];
    for (int32_t i = 0; i < 128; i++) {
        handles[i] = cmnInsert(&map, i, &result);
        TEST_ASSERT(test, result == CMN_SUCCESS);
    }

    for (int32_t i = 0; i < 128; i++) {
        bool wasHandleValid;
        int32_t element = cmnGet(&map, handles[i], &wasHandleValid);

        TEST_ASSERT(test, wasHandleValid);
        TEST_ASSERT(test, element == i);
    }
}

void checkForStaticHandleMapBucketReusage(Test* test) {
    CmnResult result;

    CmnStaticHandleMap<int32_t, 16> map;
    cmnCreateStaticHandleMap(&map, 0);

    CmnHandle first = cmnInsert(&map, 0, &result);
    TEST_ASSERT(test, result == CMN_SUCCESS);

    cmnRemove(&map, first);

    CmnHandle second = cmnInsert(&map, 1, &result);
    TEST_ASSERT(test, result == CMN_SUCCESS);

    TEST_ASSERT(test, first.index == second.index);
    TEST_ASSERT(test, (first.generation + 1) == second.generation);
}

void checkForStaticHandleMapGenerationOverflowBehaviour(Test* test) {
    CmnResult result;

    CmnStaticHandleMap<int32_t, 8> map;
    cmnCreateStaticHandleMap(&map, 0);

    // Simulate a lot of allocations and deallocations
    map.buckets[0].generation = UINT32_MAX;

    CmnHandle handle = cmnInsert(&map, 0, &result);
    TEST_ASSERT(test, handle.index == 1);
}

void checkForStaticHandleMapIndexOverflowBehaviour(Test* test) {
    CmnResult result;

    CmnStaticHandleMap<int32_t, 4> map;
    cmnCreateStaticHandleMap(&map, 0);

    // Simulate an invalid free pointer
    map.firstFree = UINT32_MAX;

    CmnHandle handle = cmnInsert(&map, 0, &result);
    TEST_ASSERT(test, handle.index == 0 && handle.generation == 0);
    TEST_ASSERT(test, result == CMN_OUT_OF_RESOURCE_SLOTS);
}

void checkForStaticHandleMapInvalidHandleBehaviour(Test* test) {

    CmnStaticHandleMap<int32_t, 32> map;
    cmnCreateStaticHandleMap(&map, 42);

    bool wasHandleValid;
    int32_t element = cmnGet(&map, { 10, 20 }, &wasHandleValid);

    TEST_ASSERT(test, !wasHandleValid);
    TEST_ASSERT(test, element == 42);
}
