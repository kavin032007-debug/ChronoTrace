#include "graph.h"
#include <stdlib.h>
#include <string.h>

ct_status_t ct_graph_init(ct_graph_t *g, size_t initial_nodes, size_t initial_edges) {
    if (!g) return CT_ERR_INVARG;

    ct_status_t st = ct_vector_init(&g->nodes, sizeof(ct_graph_node_t), initial_nodes);
    if (st != CT_OK) return st;

    st = ct_vector_init(&g->edges, sizeof(ct_graph_edge_t), initial_edges);
    if (st != CT_OK) {
        ct_vector_free(&g->nodes);
        return st;
    }

    st = ct_hashmap_init(&g->id_to_index, initial_nodes * 2);
    if (st != CT_OK) {
        ct_vector_free(&g->nodes);
        ct_vector_free(&g->edges);
        return st;
    }

    return CT_OK;
}

ct_status_t ct_graph_add_node(ct_graph_t *g, const char *id, const char *label, const char *type,
                              ct_node_kind_t kind, int64_t timestamp, int32_t severity,
                              int32_t *out_node_index) {
    if (!g || !id) return CT_ERR_INVARG;

    void *existing_val = NULL;
    if (ct_hashmap_get(&g->id_to_index, id, &existing_val) == CT_OK) {
        int32_t existing_idx = (int32_t)(intptr_t)existing_val;
        if (out_node_index) *out_node_index = existing_idx;
        return CT_OK;
    }

    int32_t new_index = (int32_t)g->nodes.size;

    ct_graph_node_t node;
    memset(&node, 0, sizeof(node));
    strncpy(node.id, id, CT_MAX_ID_LEN - 1);
    strncpy(node.label, label ? label : id, CT_MAX_NAME_LEN - 1);
    strncpy(node.type, type ? type : "", CT_MAX_TYPE_LEN - 1);
    node.kind = kind;
    node.timestamp = timestamp;
    node.severity = severity;
    node.first_edge = -1;
    node.degree = 0;

    ct_status_t st = ct_vector_push(&g->nodes, &node);
    if (st != CT_OK) return st;

    st = ct_hashmap_put(&g->id_to_index, id, (void*)(intptr_t)new_index, false);
    if (st != CT_OK) return st;

    if (out_node_index) *out_node_index = new_index;
    return CT_OK;
}

ct_status_t ct_graph_find_node(const ct_graph_t *g, const char *id, int32_t *out_node_index) {
    if (!g || !id || !out_node_index) return CT_ERR_INVARG;

    void *val = NULL;
    ct_status_t st = ct_hashmap_get(&g->id_to_index, id, &val);
    if (st != CT_OK) return CT_ERR_NOTFOUND;

    *out_node_index = (int32_t)(intptr_t)val;
    return CT_OK;
}

const ct_graph_node_t* ct_graph_get_node(const ct_graph_t *g, int32_t node_index) {
    if (!g || node_index < 0 || (size_t)node_index >= g->nodes.size) return NULL;
    return (const ct_graph_node_t*)ct_vector_get(&g->nodes, (size_t)node_index);
}

size_t ct_graph_node_count(const ct_graph_t *g) {
    return g ? g->nodes.size : 0;
}

static ct_status_t ct_graph_add_half_edge(ct_graph_t *g, int32_t source_idx, int32_t target_idx,
                                          const char *label, const char *record_id,
                                          double weight, bool is_inferred, bool is_directed) {
    ct_graph_node_t *src_node = (ct_graph_node_t*)ct_vector_get(&g->nodes, (size_t)source_idx);
    if (!src_node) return CT_ERR_INVARG;

    int32_t edge_index = (int32_t)g->edges.size;

    ct_graph_edge_t edge;
    memset(&edge, 0, sizeof(edge));
    edge.target_node = target_idx;
    edge.next_edge = src_node->first_edge;
    strncpy(edge.label, label ? label : "", CT_MAX_RELATION_LEN - 1);
    strncpy(edge.record_id, record_id ? record_id : "", CT_MAX_ID_LEN - 1);
    edge.weight = (weight >= 0.0) ? weight : 1.0;
    edge.is_inferred = is_inferred;
    edge.is_directed = is_directed;

    ct_status_t st = ct_vector_push(&g->edges, &edge);
    if (st != CT_OK) return st;

    /* Re-fetch src_node in case g->nodes reallocated (should not happen if nodes added first, but safe) */
    src_node = (ct_graph_node_t*)ct_vector_get(&g->nodes, (size_t)source_idx);
    src_node->first_edge = edge_index;
    src_node->degree++;

    return CT_OK;
}

ct_status_t ct_graph_add_edge(ct_graph_t *g, int32_t source_idx, int32_t target_idx,
                              const char *label, const char *record_id,
                              double weight, bool is_inferred, bool is_directed) {
    if (!g) return CT_ERR_INVARG;
    if (source_idx < 0 || (size_t)source_idx >= g->nodes.size) return CT_ERR_INVARG;
    if (target_idx < 0 || (size_t)target_idx >= g->nodes.size) return CT_ERR_INVARG;

    ct_status_t st = ct_graph_add_half_edge(g, source_idx, target_idx, label, record_id, weight, is_inferred, is_directed);
    if (st != CT_OK) return st;

    if (!is_directed) {
        st = ct_graph_add_half_edge(g, target_idx, source_idx, label, record_id, weight, is_inferred, false);
    }
    return st;
}

const ct_graph_edge_t* ct_graph_get_edge(const ct_graph_t *g, int32_t edge_index) {
    if (!g || edge_index < 0 || (size_t)edge_index >= g->edges.size) return NULL;
    return (const ct_graph_edge_t*)ct_vector_get(&g->edges, (size_t)edge_index);
}

size_t ct_graph_edge_count(const ct_graph_t *g) {
    return g ? g->edges.size : 0;
}

void ct_graph_clear(ct_graph_t *g) {
    if (!g) return;
    ct_vector_clear(&g->nodes);
    ct_vector_clear(&g->edges);
    ct_hashmap_clear(&g->id_to_index, NULL);
}

void ct_graph_free(ct_graph_t *g) {
    if (!g) return;
    ct_vector_free(&g->nodes);
    ct_vector_free(&g->edges);
    ct_hashmap_free(&g->id_to_index, NULL);
}
