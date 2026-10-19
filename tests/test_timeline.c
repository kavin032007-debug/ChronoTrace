#include "test_framework.h"
#include "../engine/order/timeline.h"

int test_timeline_main(void) {
    printf("--- Running Timeline & Burst Detection Tests ---\n");

    ct_timeline_t tl;
    ct_timeline_init(&tl, 10);

    /* 5 events: 3 events in rapid succession (100, 110, 120), then a pause (500), then another (510) */
    ct_event_t e1 = { .event_id = "E1", .timestamp_epoch = 100, .severity = 1 };
    ct_event_t e2 = { .event_id = "E2", .timestamp_epoch = 110, .severity = 2 };
    ct_event_t e3 = { .event_id = "E3", .timestamp_epoch = 120, .severity = 3 };
    ct_event_t e4 = { .event_id = "E4", .timestamp_epoch = 500, .severity = 1 };
    ct_event_t e5 = { .event_id = "E5", .timestamp_epoch = 510, .severity = 1 };

    ct_timeline_add(&tl, &e1);
    ct_timeline_add(&tl, &e2);
    ct_timeline_add(&tl, &e3);
    ct_timeline_add(&tl, &e4);
    ct_timeline_add(&tl, &e5);

    ct_timeline_sort(&tl);
    TEST_ASSERT(tl.is_sorted, "Timeline is sorted");

    /* Burst detection: window 50s, count threshold 3 */
    ct_burst_window_t *bursts = NULL;
    size_t burst_count = 0;
    TEST_ASSERT(ct_timeline_detect_bursts(&tl, 50, 3, 0, &bursts, &burst_count) == CT_OK, "Detect bursts in timeline");
    TEST_ASSERT(burst_count >= 1, "Detected at least 1 burst window");
    if (burst_count >= 1) {
        TEST_ASSERT(bursts[0].event_count == 3, "Burst has 3 events");
        TEST_ASSERT(bursts[0].max_severity == 3, "Max severity in burst is 3");
    }
    ct_burst_windows_free(bursts, burst_count);

    ct_timeline_free(&tl);
    return g_tests_failed;
}

#ifndef TEST_ALL_RUNNER
int main(void) {
    test_timeline_main();
    return g_tests_failed ? 1 : 0;
}
#endif
