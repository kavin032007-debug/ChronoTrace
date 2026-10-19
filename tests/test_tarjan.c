#include "test_framework.h"
#include "../engine/graph/tarjan.h"

int test_tarjan_main(void) {
    printf("--- Running Tarjan Linking-Points Tests ---\n");
    ct_graph_t g;
    ct_graph_init(&g, 5, 6);

    int32_t n0 = -1, n1 = -1, n2 = -1, n3 = -1, n4 = -1;
    ct_graph_add_node(&g, "N0", "Node 0", "type", CT_NODE_ENTITY, 0, 0, &n0);
    ct_graph_add_node(&g, "N1", "Node 1", "type", CT_NODE_ENTITY, 0, 0, &n1);
    ct_graph_add_node(&g, "N2", "Node 2", "type", CT_NODE_ENTITY, 0, 0, &n2);
    ct_graph_add_node(&g, "N3", "Node 3", "type", CT_NODE_ENTITY, 0, 0, &n3);
    ct_graph_add_node(&g, "N4", "Node 4", "type", CT_NODE_ENTITY, 0, 0, &n4);

    /* Edges: N0-N1 (bridge), N1-N2 (bridge), triangle N2-N3-N4 */
    ct_graph_add_edge(&g, n0, n1, "rel", "R01", 1.0, false, false);
    ct_graph_add_edge(&g, n1, n2, "rel", "R12", 1.0, false, false);
    ct_graph_add_edge(&g, n2, n3, "rel", "R23", 1.0, false, false);
    ct_graph_add_edge(&g, n3, n4, "rel", "R34", 1.0, false, false);
    ct_graph_add_edge(&g, n4, n2, "rel", "R42", 1.0, false, false);

    ct_tarjan_result_t res;
    TEST_ASSERT(ct_tarjan_analyze(&g, &res) == CT_OK, "Tarjan low-link analysis");

    /* N1 and N2 are articulation points! Removing N1 isolates N0; removing N2 isolates N2-N3-N4 triangle from N1-N0 */
    bool n1_is_ap = false, n2_is_ap = false;
    for (size_t i = 0; i < res.articulation_count; i++) {
        if (res.articulation_nodes[i] == n1) n1_is_ap = true;
        if (res.articulation_nodes[i] == n2) n2_is_ap = true;
    }
    TEST_ASSERT(n1_is_ap, "N1 identified as articulation point");
    TEST_ASSERT(n2_is_ap, "N2 identified as articulation point");

    /* Bridges: R01 and R12 are bridges */
    bool r01_bridge = false, r12_bridge = false;
    for (size_t i = 0; i < res.bridge_count; i++) {
        if (strcmp(res.bridges[i].record_id, "R01") == 0) r01_bridge = true;
        if (strcmp(res.bridges[i].record_id, "R12") == 0) r12_bridge = true;
    }
    TEST_ASSERT(r01_bridge, "R01 is identified as a bridge relationship");
    TEST_ASSERT(r12_bridge, "R12 is identified as a bridge relationship");

    ct_tarjan_result_free(&res);
    ct_graph_free(&g);
    return g_tests_failed;
}

#ifndef TEST_ALL_RUNNER
int main(void) {
    test_tarjan_main();
    return g_tests_failed ? 1 : 0;
}
#endif
