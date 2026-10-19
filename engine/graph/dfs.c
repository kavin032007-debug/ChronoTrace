#include "dfs.h"
#include <stdlib.h>
#include <string.h>

typedef struct {
    int32_t node;
    int32_t current_edge;
    uint32_t depth;
} ct_dfs_frame_t;

ct_status_t ct_dfs_traverse(const ct_graph_t *g, int32_t start_node, uint32_t max_depth,
                            ct_dfs_result_t *out_result) {
    if (!g || !out_result) return CT_ERR_INVARG;
    memset(out_result, 0, sizeof(ct_dfs_result_t));

    size_t n = ct_graph_node_count(g);
    if (start_node < 0 || (size_t)start_node >= n) return CT_ERR_NOTFOUND;

    out_result->discovery_time = (int32_t*)malloc(n * sizeof(int32_t));
    out_result->finish_time = (int32_t*)malloc(n * sizeof(int32_t));
    out_result->visited_order = (int32_t*)malloc(n * sizeof(int32_t));

    bool *visited = (bool*)calloc(n, sizeof(bool));
    bool *in_path = (bool*)calloc(n, sizeof(bool));

    if (!out_result->discovery_time || !out_result->finish_time ||
        !out_result->visited_order || !visited || !in_path) {
        free(out_result->discovery_time);
        free(out_result->finish_time);
        free(out_result->visited_order);
        free(visited);
        free(in_path);
        memset(out_result, 0, sizeof(ct_dfs_result_t));
        return CT_ERR_NOMEM;
    }

    for (size_t i = 0; i < n; i++) {
        out_result->discovery_time[i] = -1;
        out_result->finish_time[i] = -1;
    }

    /* Stack of frames for iterative traversal */
    size_t stack_cap = (max_depth > 0 && max_depth < n) ? (max_depth + 2) : (n + 2);
    ct_dfs_frame_t *stack = (ct_dfs_frame_t*)malloc(stack_cap * sizeof(ct_dfs_frame_t));
    if (!stack) {
        free(out_result->discovery_time);
        free(out_result->finish_time);
        free(out_result->visited_order);
        free(visited);
        free(in_path);
        memset(out_result, 0, sizeof(ct_dfs_result_t));
        return CT_ERR_NOMEM;
    }

    size_t top = 0;
    int32_t timer = 0;
    size_t visited_count = 0;
    uint32_t cycles = 0;

    /* Push start frame */
    const ct_graph_node_t *s_node = ct_graph_get_node(g, start_node);
    stack[top].node = start_node;
    stack[top].current_edge = s_node ? s_node->first_edge : -1;
    stack[top].depth = 0;
    top++;

    visited[start_node] = true;
    in_path[start_node] = true;
    out_result->discovery_time[start_node] = ++timer;
    out_result->visited_order[visited_count++] = start_node;

    while (top > 0) {
        ct_dfs_frame_t *cur = &stack[top - 1];
        int32_t u = cur->node;

        if (cur->depth >= max_depth && max_depth > 0) {
            /* Max chain depth reached for this path, finish frame */
            out_result->finish_time[u] = ++timer;
            in_path[u] = false;
            top--;
            continue;
        }

        bool advanced = false;
        while (cur->current_edge >= 0) {
            int32_t edge_idx = cur->current_edge;
            const ct_graph_edge_t *edge = ct_graph_get_edge(g, edge_idx);
            if (!edge) break;

            cur->current_edge = edge->next_edge;
            int32_t v = edge->target_node;

            if (in_path[v]) {
                cycles++;
            }

            if (!visited[v]) {
                visited[v] = true;
                in_path[v] = true;
                out_result->discovery_time[v] = ++timer;
                out_result->visited_order[visited_count++] = v;

                const ct_graph_node_t *v_node = ct_graph_get_node(g, v);
                if (top >= stack_cap) {
                    stack_cap *= 2;
                    ct_dfs_frame_t *new_stack = (ct_dfs_frame_t*)realloc(stack, stack_cap * sizeof(ct_dfs_frame_t));
                    if (!new_stack) {
                        /* On memory limit, terminate exploration */
                        break;
                    }
                    stack = new_stack;
                    cur = &stack[top - 1];
                }

                stack[top].node = v;
                stack[top].current_edge = v_node ? v_node->first_edge : -1;
                stack[top].depth = cur->depth + 1;
                top++;
                advanced = true;
                break;
            }
        }

        if (!advanced) {
            /* Finished exploring node u */
            out_result->finish_time[u] = ++timer;
            in_path[u] = false;
            top--;
        }
    }

    free(stack);
    free(visited);
    free(in_path);

    out_result->visited_count = visited_count;
    out_result->cycles_detected = cycles;
    return CT_OK;
}

void ct_dfs_result_free(ct_dfs_result_t *res) {
    if (!res) return;
    if (res->discovery_time) free(res->discovery_time);
    if (res->finish_time) free(res->finish_time);
    if (res->visited_order) free(res->visited_order);
    memset(res, 0, sizeof(ct_dfs_result_t));
}
