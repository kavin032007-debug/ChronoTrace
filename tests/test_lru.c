#include "test_framework.h"
#include "../engine/core/lru.h"

int test_lru_main(void) {
    printf("--- Running LRU Cache Tests ---\n");
    ct_lru_t lru;
    TEST_ASSERT(ct_lru_init(&lru, 2) == CT_OK, "Init LRU cache with capacity 2");

    TEST_ASSERT(ct_lru_put(&lru, "q1", "result_1") == CT_OK, "Put q1");
    TEST_ASSERT(ct_lru_put(&lru, "q2", "result_2") == CT_OK, "Put q2");

    const char *res = NULL;
    TEST_ASSERT(ct_lru_get(&lru, "q1", &res) == CT_OK && strcmp(res, "result_1") == 0, "Get q1 (moves q1 to MRU)");

    /* Insert q3, should evict q2 since q1 was accessed */
    TEST_ASSERT(ct_lru_put(&lru, "q3", "result_3") == CT_OK, "Put q3 (evicts q2)");
    TEST_ASSERT(ct_lru_get(&lru, "q2", &res) == CT_ERR_NOTFOUND, "q2 was evicted");
    TEST_ASSERT(ct_lru_get(&lru, "q1", &res) == CT_OK, "q1 is still present");

    /* Version increment invalidates cache */
    ct_lru_set_version(&lru, 2);
    TEST_ASSERT(ct_lru_get(&lru, "q1", &res) == CT_ERR_NOTFOUND, "q1 invalidated after dataset version change");

    ct_lru_free(&lru);
    return g_tests_failed;
}

#ifndef TEST_ALL_RUNNER
int main(void) {
    test_lru_main();
    return g_tests_failed ? 1 : 0;
}
#endif
