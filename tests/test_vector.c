#include "test_framework.h"
#include "../engine/core/vector.h"

int test_vector_main(void) {
    printf("--- Running Vector Tests ---\n");
    ct_vector_t vec;
    TEST_ASSERT(ct_vector_init(&vec, sizeof(int), 2) == CT_OK, "Init vector with cap 2");

    for (int i = 0; i < 10; i++) {
        TEST_ASSERT(ct_vector_push(&vec, &i) == CT_OK, "Push integer");
    }
    TEST_ASSERT(vec.size == 10, "Vector size should be 10");
    TEST_ASSERT(vec.capacity >= 10, "Vector capacity grew dynamically");

    int *val = (int*)ct_vector_get(&vec, 4);
    TEST_ASSERT(val && *val == 4, "Vector get index 4 returns 4");

    int popped = 0;
    TEST_ASSERT(ct_vector_pop(&vec, &popped) == CT_OK, "Pop last element");
    TEST_ASSERT(popped == 9, "Popped element is 9");
    TEST_ASSERT(vec.size == 9, "Vector size decreased to 9");

    ct_vector_free(&vec);
    TEST_ASSERT(vec.size == 0 && vec.data == NULL, "Vector freed properly");
    return g_tests_failed;
}

#ifndef TEST_ALL_RUNNER
int main(void) {
    test_vector_main();
    return g_tests_failed ? 1 : 0;
}
#endif
