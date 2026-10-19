#include "test_framework.h"
#include "../engine/order/kway.h"

int test_kway_main(void) {
    printf("--- Running K-Way Timeline Merge Tests ---\n");

    ct_event_t s1[2];
    memset(s1, 0, sizeof(s1));
    strcpy(s1[0].event_id, "E1"); s1[0].timestamp_epoch = 10;
    strcpy(s1[1].event_id, "E4"); s1[1].timestamp_epoch = 40;

    ct_event_t s2[2];
    memset(s2, 0, sizeof(s2));
    strcpy(s2[0].event_id, "E2"); s2[0].timestamp_epoch = 20;
    strcpy(s2[1].event_id, "E5"); s2[1].timestamp_epoch = 50;

    ct_event_t s3[1];
    memset(s3, 0, sizeof(s3));
    strcpy(s3[0].event_id, "E3"); s3[0].timestamp_epoch = 30;

    ct_event_stream_t streams[3] = {
        { .events = s1, .count = 2 },
        { .events = s2, .count = 2 },
        { .events = s3, .count = 1 }
    };

    ct_event_t merged[5];
    size_t out_count = 0;
    TEST_ASSERT(ct_kway_merge_events(streams, 3, merged, 5, &out_count) == CT_OK, "K-way merge 3 streams");
    TEST_ASSERT(out_count == 5, "Merged total count is 5");

    TEST_ASSERT(merged[0].timestamp_epoch == 10 && merged[1].timestamp_epoch == 20 &&
                merged[2].timestamp_epoch == 30 && merged[3].timestamp_epoch == 40 &&
                merged[4].timestamp_epoch == 50, "Merged events are strictly in chronological order");

    return g_tests_failed;
}

#ifndef TEST_ALL_RUNNER
int main(void) {
    test_kway_main();
    return g_tests_failed ? 1 : 0;
}
#endif
