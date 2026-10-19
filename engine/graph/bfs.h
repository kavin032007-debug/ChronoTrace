#ifndef CHRONOTRACE_BFS_H
#define CHRONOTRACE_BFS_H

#include "graph.h"
#include "../core/queue.h"

typedef struct {
    int32_t *nodes;
    int32_t *distances;
    size_t count;
} ct_bfs_level_result_t;

/* Explores neighborhood up to depth_limit. Returns array of visited node indices and their levels. */
ct_status_t ct_bfs_explore(const ct_graph_t *g, int32_t start_node, uint32_t depth_limit,
                           int32_t **out_nodes, int32_t **out_distances, size_t *out_count);

/* Finds fewest-hop path between start_node and target_node with supporting record IDs */
ct_status_t ct_bfs_shortest_path(const ct_graph_t *g, int32_t start_node, int32_t target_node,
                                 ct_path_result_t *out_path);

void ct_path_result_free(ct_path_result_t *path);

#endif /* CHRONOTRACE_BFS_H */
