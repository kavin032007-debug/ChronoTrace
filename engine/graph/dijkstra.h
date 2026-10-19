#ifndef CHRONOTRACE_DIJKSTRA_H
#define CHRONOTRACE_DIJKSTRA_H

#include "graph.h"
#include "../order/minheap.h"

ct_status_t ct_dijkstra_shortest_path(const ct_graph_t *g, int32_t start_node, int32_t target_node,
                                      ct_path_result_t *out_path);

#endif /* CHRONOTRACE_DIJKSTRA_H */
