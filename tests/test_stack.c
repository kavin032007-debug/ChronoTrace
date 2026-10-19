#include "test_framework.h"
#include "../engine/core/stack.h"

int test_stack_main(void) {
    printf("--- Running Stack Tests ---\n");
    ct_stack_t s;
    TEST_ASSERT(ct_stack_init(&s, 2) == CT_OK, "Init stack");
    TEST_ASSERT(ct_stack_is_empty(&s), "Stack is initially empty");

    TEST_ASSERT(ct_stack_push(&s, 100) == CT_OK, "Push 100");
    TEST_ASSERT(ct_stack_push(&s, 200) == CT_OK, "Push 200");
    TEST_ASSERT(ct_stack_push(&s, 300) == CT_OK, "Push 300 (grows capacity)");

    int32_t val = 0;
    TEST_ASSERT(ct_stack_peek(&s, &val) == CT_OK && val == 300, "Peek returns 300");
    TEST_ASSERT(ct_stack_pop(&s, &val) == CT_OK && val == 300, "Pop returns 300");
    TEST_ASSERT(ct_stack_pop(&s, &val) == CT_OK && val == 200, "Pop returns 200");
    TEST_ASSERT(ct_stack_pop(&s, &val) == CT_OK && val == 100, "Pop returns 100");
    TEST_ASSERT(ct_stack_is_empty(&s), "Stack is empty after popping all");
    TEST_ASSERT(ct_stack_pop(&s, &val) == CT_ERR_EMPTY, "Pop on empty stack returns CT_ERR_EMPTY");

    ct_stack_free(&s);
    return g_tests_failed;
}

#ifndef TEST_ALL_RUNNER
int main(void) {
    test_stack_main();
    return g_tests_failed ? 1 : 0;
}
#endif
