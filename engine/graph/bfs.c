#include "bfs.h"
#include <stdlib.h>
#include <string.h>

ct_status_t ct_bfs_explore(const ct_graph_t *g, int32_t start_node, uint32_t depth_limit,
                           int32_t **out_nodes, int32_t **out_distances, size_t *out_count) {
    if (!g || !out_nodes || !out_distances || !out_count) return CT_ERR_INVARG;
    size_t n = ct_graph_node_count(g);
    if (start_node < 0 || (size_t)start_node >= n) return CT_ERR_NOTFOUND;

    int32_t *dist = (int32_t*)malloc(n * sizeof(int32_t));
    if (!dist) return CT_ERR_NOMEM;
    for (size_t i = 0; i < n; i++) dist[i] = -1;

    ct_queue_t q;
    ct_status_t st = ct_queue_init(&q, 64);
    if (st != CT_OK) {
        free(dist);
        return st;
    }

    int32_t *visited_nodes = (int32_t*)malloc(n * sizeof(int32_t));
    int32_t *visited_dist = (int32_t*)malloc(n * sizeof(int32_t));
    if (!visited_nodes || !visited_dist) {
        free(dist);
        free(visited_nodes);
        free(visited_dist);
        ct_queue_free(&q);
        return CT_ERR_NOMEM;
    }

    size_t visited_cnt = 0;

    dist[start_node] = 0;
    ct_queue_enqueue(&q, start_node);

    while (!ct_queue_is_empty(&q)) {
        int32_t u = 0;
        ct_queue_dequeue(&q, &u);

        visited_nodes[visited_cnt] = u;
        visited_dist[visited_cnt] = dist[u];
        visited_cnt++;

        if ((uint32_t)dist[u] >= depth_limit) {
            continue;
        }

        const ct_graph_node_t *node_u = ct_graph_get_node(g, u);
        int32_t edge_idx = node_u ? node_u->first_edge : -1;

        while (edge_idx >= 0) {
            const ct_graph_edge_t *edge = ct_graph_get_edge(g, edge_idx);
            if (!edge) break;

            int32_t v = edge->target_node;
            if (dist[v] == -1) {
                dist[v] = dist[u] + 1;
                ct_queue_enqueue(&q, v);
            }
            edge_idx = edge->next_edge;
        }
    }

    ct_queue_free(&q);
    free(dist);

    *out_nodes = visited_nodes;
    *out_distances = visited_dist;
    *out_count = visited_cnt;
    return CT_OK;
}

ct_status_t ct_bfs_shortest_path(const ct_graph_t *g, int32_t start_node, int32_t target_node,
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

    int32_t *parent_node = (int32_t*)malloc(n * sizeof(int32_t));
    int32_t *parent_edge = (int32_t*)malloc(n * sizeof(int32_t));
    int32_t *dist = (int32_t*)malloc(n * sizeof(int32_t));
    if (!parent_node || !parent_edge || !dist) {
        free(parent_node);
        free(parent_edge);
        free(dist);
        return CT_ERR_NOMEM;
    }

    for (size_t i = 0; i < n; i++) {
        parent_node[i] = -1;
        parent_edge[i] = -1;
        dist[i] = -1;
    }

    ct_queue_t q;
    ct_status_t st = ct_queue_init(&q, 64);
    if (st != CT_OK) {
        free(parent_node);
        free(parent_edge);
        free(dist);
        return st;
    }

    dist[start_node] = 0;
    ct_queue_enqueue(&q, start_node);

    bool found = false;
    while (!ct_queue_is_empty(&q)) {
        int32_t u = 0;
        ct_queue_dequeue(&q, &u);

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
            if (dist[v] == -1) {
                dist[v] = dist[u] + 1;
                parent_node[v] = u;
                parent_edge[v] = edge_idx;
                ct_queue_enqueue(&q, v);
            }
            edge_idx = edge->next_edge;
        }
    }

    ct_queue_free(&q);

    if (!found) {
        free(parent_node);
        free(parent_edge);
        free(dist);
        return CT_ERR_NOTFOUND;
    }

    /* Trace path backwards */
    uint32_t hops = (uint32_t)dist[target_node];
    out_path->node_path = (int32_t*)malloc((hops + 1) * sizeof(int32_t));
    out_path->record_ids = (char(*)[CT_MAX_ID_LEN])malloc(hops * sizeof(char[CT_MAX_ID_LEN]));
    if (!out_path->node_path || !out_path->record_ids) {
        free(parent_node);
        free(parent_edge);
        free(dist);
        ct_path_result_free(out_path);
        return CT_ERR_NOMEM;
    }

    out_path->hop_count = hops;
    out_path->total_cost = (double)hops;

    int32_t curr = target_node;
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

    free(parent_node);
    free(parent_edge);
    free(dist);
    return CT_OK;
}

void ct_path_result_free(ct_path_result_t *path) {
    if (!path) return;
    if (path->node_path) {
        free(path->node_path);
        path->node_path = NULL;
    }
    if (path->record_ids) {
        free(path->record_ids);
        path->record_ids = NULL;
    }
    path->hop_count = 0;
    path->total_cost = 0.0;
}
