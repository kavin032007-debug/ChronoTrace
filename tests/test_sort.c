#include "test_framework.h"
#include "../engine/order/sort.h"

int test_sort_main(void) {
    printf("--- Running Stable Merge Sort Tests ---\n");

    /* Test array with duplicate timestamps to verify stability and secondary tie breaker */
    ct_event_t evs[4];
    memset(evs, 0, sizeof(evs));

    strcpy(evs[0].event_id, "E200");
    evs[0].timestamp_epoch = 1000;

    strcpy(evs[1].event_id, "E100");
    evs[1].timestamp_epoch = 1000; /* Same timestamp, but smaller ID */

    strcpy(evs[2].event_id, "E050");
    evs[2].timestamp_epoch = 500;  /* Earlier timestamp */

    strcpy(evs[3].event_id, "E300");
    evs[3].timestamp_epoch = 2000; /* Later timestamp */

    TEST_ASSERT(ct_stable_merge_sort_events(evs, 4) == CT_OK, "Sort events array");

    TEST_ASSERT_STR_EQ(evs[0].event_id, "E050", "Index 0 is E050 (epoch 500)");
    TEST_ASSERT_STR_EQ(evs[1].event_id, "E100", "Index 1 is E100 (epoch 1000, tie-broken by ID)");
    TEST_ASSERT_STR_EQ(evs[2].event_id, "E200", "Index 2 is E200 (epoch 1000)");
    TEST_ASSERT_STR_EQ(evs[3].event_id, "E300", "Index 3 is E300 (epoch 2000)");

    return g_tests_failed;
}

#ifndef TEST_ALL_RUNNER
int main(void) {
    test_sort_main();
    return g_tests_failed ? 1 : 0;
}
#endif
