#include "dijkstra.h"
#include <stdlib.h>
#include <string.h>
#include <float.h>

ct_status_t ct_dijkstra_shortest_path(const ct_graph_t *g, int32_t start_node, int32_t target_node,
                                      ct_path_result_t *out_path) {
    if (!g || !out_path) return CT_ERR_INVARG;
    memset(out_path, 0, sizeof(ct_path_result_t));

    size_t n = ct_graph_node_count(g);
    if (start_node < 0 || (size_t)start_node >= n) return CT_ERR_NOTFOUND;
    if (target_node < 0 || (size_t)target_node >= n) return CT_ERR_NOTFOUND;

    if (start_node == target_node) {
        out_path->node_path = (int32_t*)malloc(sizeof(int32_t));
        if (!out_path->node_path) return CT_ERR_NOMEM;
        out_path->node_path[0] = start_node;
        out_path->record_ids = NULL;
        out_path->hop_count = 0;
        out_path->total_cost = 0.0;
        return CT_OK;
    }

    double *dist = (double*)malloc(n * sizeof(double));
    int32_t *parent_node = (int32_t*)malloc(n * sizeof(int32_t));
    int32_t *parent_edge = (int32_t*)malloc(n * sizeof(int32_t));
    bool *visited = (bool*)calloc(n, sizeof(bool));

    if (!dist || !parent_node || !parent_edge || !visited) {
        free(dist);
        free(parent_node);
        free(parent_edge);
        free(visited);
        return CT_ERR_NOMEM;
    }

    for (size_t i = 0; i < n; i++) {
        dist[i] = DBL_MAX;
        parent_node[i] = -1;
        parent_edge[i] = -1;
    }

    ct_minheap_t heap;
    ct_status_t st = ct_minheap_init(&heap, 64, n, NULL);
    if (st != CT_OK) {
        free(dist);
        free(parent_node);
        free(parent_edge);
        free(visited);
        return st;
    }

    dist[start_node] = 0.0;
    ct_minheap_insert(&heap, start_node, 0.0, NULL);

    bool found = false;
    while (!ct_minheap_is_empty(&heap)) {
        ct_heap_node_t min_elem;
        ct_minheap_extract_min(&heap, &min_elem);
        int32_t u = min_elem.id;

        if (visited[u]) continue;
        visited[u] = true;

        if (u == target_node) {
            found = true;
            break;
        }

        const ct_graph_node_t *node_u = ct_graph_get_node(g, u);
        int32_t edge_idx = node_u ? node_u->first_edge : -1;

        while (edge_idx >= 0) {
            const ct_graph_edge_t *edge = ct_graph_get_edge(g, edge_idx);
            if (!edge) break;

            int32_t v = edge->target_node;
            if (!visited[v]) {
                double weight = edge->weight;
                if (weight < 0.0) weight = 0.0; /* Ensure non-negative */

                if (dist[u] + weight < dist[v]) {
                    dist[v] = dist[u] + weight;
                    parent_node[v] = u;
                    parent_edge[v] = edge_idx;

                    if (ct_minheap_contains(&heap, v)) {
                        ct_minheap_decrease_key(&heap, v, dist[v]);
                    } else {
                        ct_minheap_insert(&heap, v, dist[v], NULL);
                    }
                }
            }
            edge_idx = edge->next_edge;
        }
    }

    ct_minheap_free(&heap);

    if (!found || dist[target_node] == DBL_MAX) {
        free(dist);
        free(parent_node);
        free(parent_edge);
        free(visited);
        return CT_ERR_NOTFOUND;
    }

    /* Count hops */
    uint32_t hops = 0;
    int32_t curr = target_node;
    while (curr != start_node) {
        hops++;
        curr = parent_node[curr];
        if (curr == -1) break;
    }

    out_path->node_path = (int32_t*)malloc((hops + 1) * sizeof(int32_t));
    out_path->record_ids = (char(*)[CT_MAX_ID_LEN])malloc(hops * sizeof(char[CT_MAX_ID_LEN]));
    if (!out_path->node_path || !out_path->record_ids) {
        free(dist);
        free(parent_node);
        free(parent_edge);
        free(visited);
        free(out_path->node_path);
        free(out_path->record_ids);
        memset(out_path, 0, sizeof(ct_path_result_t));
        return CT_ERR_NOMEM;
    }

    out_path->hop_count = hops;
    out_path->total_cost = dist[target_node];

    curr = target_node;
    for (int32_t h = (int32_t)hops; h > 0; h--) {
        out_path->node_path[h] = curr;
        int32_t p_edge_idx = parent_edge[curr];
        const ct_graph_edge_t *edge = ct_graph_get_edge(g, p_edge_idx);
        if (edge) {
            strncpy(out_path->record_ids[h - 1], edge->record_id, CT_MAX_ID_LEN - 1);
            out_path->record_ids[h - 1][CT_MAX_ID_LEN - 1] = '\0';
        } else {
            out_path->record_ids[h - 1][0] = '\0';
        }
        curr = parent_node[curr];
    }
    out_path->node_path[0] = start_node;

    free(dist);
    free(parent_node);
    free(parent_edge);
    free(visited);
    return CT_OK;
}
