#include "test_framework.h"
#include "../engine/graph/graph.h"

int test_graph_main(void) {
    printf("--- Running Graph Tests ---\n");
    ct_graph_t g;
    TEST_ASSERT(ct_graph_init(&g, 4, 4) == CT_OK, "Init graph");

    int32_t idx_u = -1, idx_d = -1, idx_f = -1;
    TEST_ASSERT(ct_graph_add_node(&g, "U01", "User A", "user", CT_NODE_ENTITY, 0, 0, &idx_u) == CT_OK, "Add node U01");
    TEST_ASSERT(ct_graph_add_node(&g, "D01", "Computer 01", "device", CT_NODE_ENTITY, 0, 0, &idx_d) == CT_OK, "Add node D01");
    TEST_ASSERT(ct_graph_add_node(&g, "F01", "Project.docx", "file", CT_NODE_ENTITY, 0, 0, &idx_f) == CT_OK, "Add node F01");

    /* Adding existing ID returns same index */
    int32_t dup_idx = -1;
    TEST_ASSERT(ct_graph_add_node(&g, "U01", "User A", "user", CT_NODE_ENTITY, 0, 0, &dup_idx) == CT_OK && dup_idx == idx_u, "Duplicate node returns same index");

    TEST_ASSERT(ct_graph_add_edge(&g, idx_u, idx_d, "logged_into", "R01", 1.0, false, false) == CT_OK, "Add edge U01-D01");
    TEST_ASSERT(ct_graph_add_edge(&g, idx_d, idx_f, "accessed", "R02", 1.0, false, false) == CT_OK, "Add edge D01-F01");

    TEST_ASSERT(ct_graph_node_count(&g) == 3, "Node count is 3");
    TEST_ASSERT(ct_graph_edge_count(&g) == 4, "Edge count is 4 (2 undirected edges = 4 half edges)");

    const ct_graph_node_t *node_d = ct_graph_get_node(&g, idx_d);
    TEST_ASSERT(node_d && node_d->degree == 2, "Node D01 degree is 2");

    ct_graph_free(&g);
    return g_tests_failed;
}

#ifndef TEST_ALL_RUNNER
int main(void) {
    test_graph_main();
    return g_tests_failed ? 1 : 0;
}
#endif
