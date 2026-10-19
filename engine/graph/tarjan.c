#include "tarjan.h"
#include <stdlib.h>
#include <string.h>

typedef struct {
    const ct_graph_t *g;
    int32_t *tin;
    int32_t *low;
    bool *visited;
    bool *is_ap;
    int32_t timer;
    ct_bridge_t *bridges;
    size_t bridge_count;
    size_t bridge_capacity;
} ct_tarjan_ctx_t;

static void tarjan_dfs(ct_tarjan_ctx_t *ctx, int32_t u, int32_t p) {
    ctx->visited[u] = true;
    ctx->tin[u] = ctx->low[u] = ++ctx->timer;
    int children = 0;

    const ct_graph_node_t *node_u = ct_graph_get_node(ctx->g, u);
    int32_t edge_idx = node_u ? node_u->first_edge : -1;

    while (edge_idx >= 0) {
        const ct_graph_edge_t *edge = ct_graph_get_edge(ctx->g, edge_idx);
        if (!edge) break;

        int32_t to = edge->target_node;
        if (to == p) {
            edge_idx = edge->next_edge;
            continue;
        }

        if (ctx->visited[to]) {
            if (ctx->tin[to] < ctx->low[u]) {
                ctx->low[u] = ctx->tin[to];
            }
        } else {
            tarjan_dfs(ctx, to, u);
            if (ctx->low[to] < ctx->low[u]) {
                ctx->low[u] = ctx->low[to];
            }

            if (ctx->low[to] >= ctx->tin[u] && p != -1) {
                ctx->is_ap[u] = true;
            }

            if (ctx->low[to] > ctx->tin[u]) {
                /* Found bridge edge */
                if (ctx->bridge_count >= ctx->bridge_capacity) {
                    size_t new_cap = ctx->bridge_capacity * 2;
                    ct_bridge_t *new_b = (ct_bridge_t*)realloc(ctx->bridges, new_cap * sizeof(ct_bridge_t));
                    if (new_b) {
                        ctx->bridges = new_b;
                        ctx->bridge_capacity = new_cap;
                    }
                }
                if (ctx->bridge_count < ctx->bridge_capacity) {
                    ct_bridge_t *br = &ctx->bridges[ctx->bridge_count++];
                    br->u = u;
                    br->v = to;
                    strncpy(br->record_id, edge->record_id, CT_MAX_ID_LEN - 1);
                    br->record_id[CT_MAX_ID_LEN - 1] = '\0';
                }
            }

            children++;
        }

        edge_idx = edge->next_edge;
    }

    if (p == -1 && children > 1) {
        ctx->is_ap[u] = true;
    }
}

ct_status_t ct_tarjan_analyze(const ct_graph_t *g, ct_tarjan_result_t *out_result) {
    if (!g || !out_result) return CT_ERR_INVARG;
    memset(out_result, 0, sizeof(ct_tarjan_result_t));

    size_t n = ct_graph_node_count(g);
    if (n == 0) return CT_OK;

    ct_tarjan_ctx_t ctx;
    ctx.g = g;
    ctx.timer = 0;
    ctx.bridge_count = 0;
    ctx.bridge_capacity = 16;
    ctx.bridges = (ct_bridge_t*)malloc(ctx.bridge_capacity * sizeof(ct_bridge_t));

    ctx.tin = (int32_t*)malloc(n * sizeof(int32_t));
    ctx.low = (int32_t*)malloc(n * sizeof(int32_t));
    ctx.visited = (bool*)calloc(n, sizeof(bool));
    ctx.is_ap = (bool*)calloc(n, sizeof(bool));

    if (!ctx.bridges || !ctx.tin || !ctx.low || !ctx.visited || !ctx.is_ap) {
        free(ctx.bridges);
        free(ctx.tin);
        free(ctx.low);
        free(ctx.visited);
        free(ctx.is_ap);
        return CT_ERR_NOMEM;
    }

    for (size_t i = 0; i < n; i++) {
        if (!ctx.visited[i]) {
            tarjan_dfs(&ctx, (int32_t)i, -1);
        }
    }

    /* Count and collect articulation points */
    size_t ap_cnt = 0;
    for (size_t i = 0; i < n; i++) {
        if (ctx.is_ap[i]) ap_cnt++;
    }

    out_result->articulation_nodes = (int32_t*)malloc(ap_cnt * sizeof(int32_t));
    if (ap_cnt > 0 && !out_result->articulation_nodes) {
        free(ctx.bridges);
        free(ctx.tin);
        free(ctx.low);
        free(ctx.visited);
        free(ctx.is_ap);
        return CT_ERR_NOMEM;
    }

    size_t ap_idx = 0;
    for (size_t i = 0; i < n; i++) {
        if (ctx.is_ap[i]) {
            out_result->articulation_nodes[ap_idx++] = (int32_t)i;
        }
    }

    out_result->articulation_count = ap_cnt;
    out_result->bridges = ctx.bridges;
    out_result->bridge_count = ctx.bridge_count;

    free(ctx.tin);
    free(ctx.low);
    free(ctx.visited);
    free(ctx.is_ap);
    return CT_OK;
}

void ct_tarjan_result_free(ct_tarjan_result_t *res) {
    if (!res) return;
    if (res->articulation_nodes) {
        free(res->articulation_nodes);
        res->articulation_nodes = NULL;
    }
    if (res->bridges) {
        free(res->bridges);
        res->bridges = NULL;
    }
    res->articulation_count = 0;
    res->bridge_count = 0;
}
