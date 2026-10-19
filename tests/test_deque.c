#include "test_framework.h"
#include "../engine/core/deque.h"

int test_deque_main(void) {
    printf("--- Running Deque Tests ---\n");
    ct_deque_t dq;
    TEST_ASSERT(ct_deque_init(&dq, 4) == CT_OK, "Init deque");

    TEST_ASSERT(ct_deque_push_back(&dq, 1) == CT_OK, "Push back 1");
    TEST_ASSERT(ct_deque_push_back(&dq, 2) == CT_OK, "Push back 2");
    TEST_ASSERT(ct_deque_push_front(&dq, 0) == CT_OK, "Push front 0");
    TEST_ASSERT(ct_deque_push_front(&dq, -1) == CT_OK, "Push front -1");

    int32_t val = 0;
    TEST_ASSERT(ct_deque_peek_front(&dq, &val) == CT_OK && val == -1, "Peek front is -1");
    TEST_ASSERT(ct_deque_peek_back(&dq, &val) == CT_OK && val == 2, "Peek back is 2");

    TEST_ASSERT(ct_deque_pop_front(&dq, &val) == CT_OK && val == -1, "Pop front is -1");
    TEST_ASSERT(ct_deque_pop_back(&dq, &val) == CT_OK && val == 2, "Pop back is 2");
    TEST_ASSERT(ct_deque_pop_front(&dq, &val) == CT_OK && val == 0, "Pop front is 0");
    TEST_ASSERT(ct_deque_pop_front(&dq, &val) == CT_OK && val == 1, "Pop front is 1");
    TEST_ASSERT(ct_deque_is_empty(&dq), "Deque is empty");

    ct_deque_free(&dq);
    return g_tests_failed;
}

#ifndef TEST_ALL_RUNNER
int main(void) {
    test_deque_main();
    return g_tests_failed ? 1 : 0;
}
#endif
