#include "test_framework.h"
#include "../engine/graph/bfs.h"

int test_bfs_main(void) {
    printf("--- Running BFS Tests ---\n");
    ct_graph_t g;
    ct_graph_init(&g, 4, 4);

    int32_t u01 = -1, d01 = -1, f01 = -1, d02 = -1;
    ct_graph_add_node(&g, "U01", "User A", "user", CT_NODE_ENTITY, 0, 0, &u01);
    ct_graph_add_node(&g, "D01", "Computer 01", "device", CT_NODE_ENTITY, 0, 0, &d01);
    ct_graph_add_node(&g, "F01", "Project.docx", "file", CT_NODE_ENTITY, 0, 0, &f01);
    ct_graph_add_node(&g, "D02", "Isolated Device", "device", CT_NODE_ENTITY, 0, 0, &d02);

    ct_graph_add_edge(&g, u01, d01, "logged_into", "R01", 1.0, false, false);
    ct_graph_add_edge(&g, d01, f01, "accessed", "R02", 1.0, false, false);

    /* BFS depth limit 1 from U01 should only reach U01 (dist 0) and D01 (dist 1) */
    int32_t *nodes = NULL;
    int32_t *dist = NULL;
    size_t count = 0;
    TEST_ASSERT(ct_bfs_explore(&g, u01, 1, &nodes, &dist, &count) == CT_OK, "BFS explore depth 1");
    TEST_ASSERT(count == 2, "Depth 1 reached exactly 2 nodes (U01 and D01)");
    free(nodes);
    free(dist);

    /* BFS path between U01 and F01 */
    ct_path_result_t path;
    TEST_ASSERT(ct_bfs_shortest_path(&g, u01, f01, &path) == CT_OK, "BFS shortest path U01 -> F01");
    TEST_ASSERT(path.hop_count == 2, "Path hop count is 2");
    TEST_ASSERT(path.node_path[0] == u01 && path.node_path[1] == d01 && path.node_path[2] == f01, "Path nodes are U01 -> D01 -> F01");
    TEST_ASSERT_STR_EQ(path.record_ids[0], "R01", "First hop record is R01");
    TEST_ASSERT_STR_EQ(path.record_ids[1], "R02", "Second hop record is R02");
    ct_path_result_free(&path);

    /* BFS path to disconnected node D02 */
    TEST_ASSERT(ct_bfs_shortest_path(&g, u01, d02, &path) == CT_ERR_NOTFOUND, "BFS path to disconnected node returns CT_ERR_NOTFOUND");

    ct_graph_free(&g);
    return g_tests_failed;
}

#ifndef TEST_ALL_RUNNER
int main(void) {
    test_bfs_main();
    return g_tests_failed ? 1 : 0;
}
#endif
