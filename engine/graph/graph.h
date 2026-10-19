#ifndef CHRONOTRACE_GRAPH_H
#define CHRONOTRACE_GRAPH_H

#include "../../include/models.h"
#include "../core/vector.h"
#include "../core/hashmap.h"

typedef struct {
    ct_vector_t nodes;      /* Array of ct_graph_node_t */
    ct_vector_t edges;      /* Array of ct_graph_edge_t (contiguous edge pool) */
    ct_hashmap_t id_to_index; /* Maps ID string -> intptr_t (node index) */
} ct_graph_t;

ct_status_t ct_graph_init(ct_graph_t *g, size_t initial_nodes, size_t initial_edges);

/* Node management */
ct_status_t ct_graph_add_node(ct_graph_t *g, const char *id, const char *label, const char *type,
                              ct_node_kind_t kind, int64_t timestamp, int32_t severity,
                              int32_t *out_node_index);
ct_status_t ct_graph_find_node(const ct_graph_t *g, const char *id, int32_t *out_node_index);
const ct_graph_node_t* ct_graph_get_node(const ct_graph_t *g, int32_t node_index);
size_t ct_graph_node_count(const ct_graph_t *g);

/* Edge management */
ct_status_t ct_graph_add_edge(ct_graph_t *g, int32_t source_idx, int32_t target_idx,
                              const char *label, const char *record_id,
                              double weight, bool is_inferred, bool is_directed);

const ct_graph_edge_t* ct_graph_get_edge(const ct_graph_t *g, int32_t edge_index);
size_t ct_graph_edge_count(const ct_graph_t *g);

void ct_graph_clear(ct_graph_t *g);
void ct_graph_free(ct_graph_t *g);

#endif /* CHRONOTRACE_GRAPH_H */
