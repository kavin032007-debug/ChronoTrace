#include "test_framework.h"
#include "../engine/graph/dfs.h"

int test_dfs_main(void) {
    printf("--- Running DFS Tests ---\n");
    ct_graph_t g;
    ct_graph_init(&g, 4, 4);

    int32_t a = -1, b = -1, c = -1;
    ct_graph_add_node(&g, "A", "Node A", "type", CT_NODE_ENTITY, 0, 0, &a);
    ct_graph_add_node(&g, "B", "Node B", "type", CT_NODE_ENTITY, 0, 0, &b);
    ct_graph_add_node(&g, "C", "Node C", "type", CT_NODE_ENTITY, 0, 0, &c);

    ct_graph_add_edge(&g, a, b, "rel", "R1", 1.0, false, true);
    ct_graph_add_edge(&g, b, c, "rel", "R2", 1.0, false, true);
    ct_graph_add_edge(&g, c, a, "rel", "R3", 1.0, false, true); /* Directed cycle */

    ct_dfs_result_t res;
    TEST_ASSERT(ct_dfs_traverse(&g, a, 10, &res) == CT_OK, "Iterative DFS traversal");
    TEST_ASSERT(res.visited_count == 3, "Visited all 3 nodes in chain");
    TEST_ASSERT(res.cycles_detected >= 1, "Detected directed cycle A -> B -> C -> A");
    TEST_ASSERT(res.discovery_time[a] > 0 && res.finish_time[a] > res.discovery_time[a], "Discovery and finish timestamps recorded");

    ct_dfs_result_free(&res);
    ct_graph_free(&g);
    return g_tests_failed;
}

#ifndef TEST_ALL_RUNNER
int main(void) {
    test_dfs_main();
    return g_tests_failed ? 1 : 0;
}
#endif
