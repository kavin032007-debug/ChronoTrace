#include "test_framework.h"
#include "../engine/order/search.h"

int test_search_main(void) {
    printf("--- Running Binary Search Range Query Tests ---\n");

    ct_event_t evs[5];
    memset(evs, 0, sizeof(evs));

    evs[0].timestamp_epoch = 100;
    evs[1].timestamp_epoch = 200;
    evs[2].timestamp_epoch = 200;
    evs[3].timestamp_epoch = 300;
    evs[4].timestamp_epoch = 500;

    int64_t lb = ct_search_lower_bound(evs, 5, 200);
    TEST_ASSERT(lb == 1, "Lower bound for 200 is index 1");

    int64_t ub = ct_search_upper_bound(evs, 5, 200);
    TEST_ASSERT(ub == 3, "Upper bound for 200 is index 3");

    /* Range query [200, 300] inclusive */
    size_t start_idx = 0, count = 0;
    TEST_ASSERT(ct_search_time_range(evs, 5, 200, 300, &start_idx, &count) == CT_OK, "Search range [200, 300]");
    TEST_ASSERT(start_idx == 1, "Start index is 1");
    TEST_ASSERT(count == 3, "Count is 3 (indices 1, 2, 3)");

    return g_tests_failed;
}

#ifndef TEST_ALL_RUNNER
int main(void) {
    test_search_main();
    return g_tests_failed ? 1 : 0;
}
#endif
