#include "test_framework.h"
#include "../engine/graph/dsu.h"

int test_dsu_main(void) {
    printf("--- Running DSU Tests ---\n");
    ct_dsu_t dsu;
    TEST_ASSERT(ct_dsu_init(&dsu, 6) == CT_OK, "Init DSU for 6 elements");
    TEST_ASSERT(dsu.num_components == 6, "Initial components count is 6");

    TEST_ASSERT(ct_dsu_union(&dsu, 0, 1), "Union 0 and 1");
    TEST_ASSERT(ct_dsu_union(&dsu, 1, 2), "Union 1 and 2");
    TEST_ASSERT(!ct_dsu_union(&dsu, 0, 2), "Union 0 and 2 returns false (already united)");

    TEST_ASSERT(ct_dsu_connected(&dsu, 0, 2), "0 and 2 are connected");
    TEST_ASSERT(!ct_dsu_connected(&dsu, 0, 3), "0 and 3 are not connected");
    TEST_ASSERT(ct_dsu_group_size(&dsu, 0) == 3, "Group size containing 0 is 3");

    TEST_ASSERT(ct_dsu_union(&dsu, 3, 4), "Union 3 and 4");
    TEST_ASSERT(dsu.num_components == 3, "Components count is now 3: {0,1,2}, {3,4}, {5}");

    ct_dsu_free(&dsu);
    return g_tests_failed;
}

#ifndef TEST_ALL_RUNNER
int main(void) {
    test_dsu_main();
    return g_tests_failed ? 1 : 0;
}
#endif
