#include "test_framework.h"
#include "../engine/graph/dijkstra.h"
#include "../engine/graph/bfs.h"

int test_dijkstra_main(void) {
    printf("--- Running Dijkstra Tests ---\n");
    ct_graph_t g;
    ct_graph_init(&g, 4, 6);

    int32_t a = -1, b = -1, c = -1;
    ct_graph_add_node(&g, "A", "Node A", "type", CT_NODE_ENTITY, 0, 0, &a);
    ct_graph_add_node(&g, "B", "Node B", "type", CT_NODE_ENTITY, 0, 0, &b);
    ct_graph_add_node(&g, "C", "Node C", "type", CT_NODE_ENTITY, 0, 0, &c);

    /* Direct path A -> C has weight 10. Indirect path A -> B -> C has weight 2 + 3 = 5 */
    ct_graph_add_edge(&g, a, c, "direct", "R_AC", 10.0, false, false);
    ct_graph_add_edge(&g, a, b, "step1", "R_AB", 2.0, false, false);
    ct_graph_add_edge(&g, b, c, "step2", "R_BC", 3.0, false, false);

    ct_path_result_t path;
    TEST_ASSERT(ct_dijkstra_shortest_path(&g, a, c, &path) == CT_OK, "Dijkstra shortest path A -> C");
    TEST_ASSERT(path.total_cost == 5.0, "Total cost is 5.0 (chose cheaper 2+3 route over 10)");
    TEST_ASSERT(path.hop_count == 2, "Path has 2 hops");
    TEST_ASSERT(path.node_path[0] == a && path.node_path[1] == b && path.node_path[2] == c, "Path is A -> B -> C");
    TEST_ASSERT_STR_EQ(path.record_ids[0], "R_AB", "Hop 1 is R_AB");
    TEST_ASSERT_STR_EQ(path.record_ids[1], "R_BC", "Hop 2 is R_BC");

    ct_path_result_free(&path);
    ct_graph_free(&g);
    return g_tests_failed;
}

#ifndef TEST_ALL_RUNNER
int main(void) {
    test_dijkstra_main();
    return g_tests_failed ? 1 : 0;
}
#endif
