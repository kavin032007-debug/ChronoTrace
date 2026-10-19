#include "test_framework.h"
#include "../engine/core/hashmap.h"

int test_hashmap_main(void) {
    printf("--- Running Hashmap Tests ---\n");
    ct_hashmap_t map;
    TEST_ASSERT(ct_hashmap_init(&map, 4) == CT_OK, "Init hashmap with 4 buckets");

    int val1 = 101, val2 = 202, val3 = 303;
    TEST_ASSERT(ct_hashmap_put(&map, "U01", &val1, false) == CT_OK, "Put U01");
    TEST_ASSERT(ct_hashmap_put(&map, "D01", &val2, false) == CT_OK, "Put D01");
    TEST_ASSERT(ct_hashmap_put(&map, "F01", &val3, false) == CT_OK, "Put F01");

    /* Duplicate key detection */
    TEST_ASSERT(ct_hashmap_put(&map, "U01", &val2, false) == CT_ERR_DUPLICATE, "Duplicate key returns CT_ERR_DUPLICATE when overwrite forbidden");

    /* Lookup */
    void *out = NULL;
    TEST_ASSERT(ct_hashmap_get(&map, "U01", &out) == CT_OK && out == &val1, "Get U01 returns val1");
    TEST_ASSERT(ct_hashmap_contains(&map, "D01"), "Contains D01");
    TEST_ASSERT(!ct_hashmap_contains(&map, "E999"), "Does not contain E999");

    /* Remove */
    void *old = NULL;
    TEST_ASSERT(ct_hashmap_remove(&map, "D01", &old) == CT_OK && old == &val2, "Remove D01");
    TEST_ASSERT(!ct_hashmap_contains(&map, "D01"), "D01 is no longer in map");
    TEST_ASSERT(ct_hashmap_size(&map) == 2, "Map size is 2");

    ct_hashmap_free(&map, NULL);
    TEST_ASSERT(ct_hashmap_size(&map) == 0, "Map freed");
    return g_tests_failed;
}

#ifndef TEST_ALL_RUNNER
int main(void) {
    test_hashmap_main();
    return g_tests_failed ? 1 : 0;
}
#endif
