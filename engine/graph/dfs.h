#ifndef CHRONOTRACE_DFS_H
#define CHRONOTRACE_DFS_H

#include "graph.h"
#include "../core/stack.h"

typedef struct {
    int32_t *discovery_time;
    int32_t *finish_time;
    int32_t *visited_order;
    size_t visited_count;
    uint32_t cycles_detected;
} ct_dfs_result_t;

/* Iterative DFS starting from start_node with maximum chain depth */
ct_status_t ct_dfs_traverse(const ct_graph_t *g, int32_t start_node, uint32_t max_depth,
                            ct_dfs_result_t *out_result);

void ct_dfs_result_free(ct_dfs_result_t *res);

#endif /* CHRONOTRACE_DFS_H */
