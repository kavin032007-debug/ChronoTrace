#include "test_framework.h"
#include "../engine/core/queue.h"

int test_queue_main(void) {
    printf("--- Running Queue Tests ---\n");
    ct_queue_t q;
    TEST_ASSERT(ct_queue_init(&q, 4) == CT_OK, "Init queue with capacity 4");
    TEST_ASSERT(ct_queue_is_empty(&q), "Queue is initially empty");

    TEST_ASSERT(ct_queue_enqueue(&q, 10) == CT_OK, "Enqueue 10");
    TEST_ASSERT(ct_queue_enqueue(&q, 20) == CT_OK, "Enqueue 20");
    TEST_ASSERT(ct_queue_enqueue(&q, 30) == CT_OK, "Enqueue 30");

    int32_t val = 0;
    TEST_ASSERT(ct_queue_peek(&q, &val) == CT_OK && val == 10, "Peek returns 10");
    TEST_ASSERT(ct_queue_dequeue(&q, &val) == CT_OK && val == 10, "Dequeue returns 10");
    TEST_ASSERT(ct_queue_dequeue(&q, &val) == CT_OK && val == 20, "Dequeue returns 20");

    TEST_ASSERT(ct_queue_enqueue(&q, 40) == CT_OK, "Enqueue 40");
    TEST_ASSERT(ct_queue_enqueue(&q, 50) == CT_OK, "Enqueue 50");
    TEST_ASSERT(ct_queue_enqueue(&q, 60) == CT_OK, "Enqueue 60 (triggers grow)");

    TEST_ASSERT(ct_queue_size(&q) == 4, "Queue size is 4");
    ct_queue_free(&q);
    TEST_ASSERT(ct_queue_is_empty(&q), "Queue freed");
    return g_tests_failed;
}

#ifndef TEST_ALL_RUNNER
int main(void) {
    test_queue_main();
    return g_tests_failed ? 1 : 0;
}
#endif
