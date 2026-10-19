#ifndef CHRONOTRACE_TARJAN_H
#define CHRONOTRACE_TARJAN_H

#include "graph.h"

typedef struct {
    int32_t u;
    int32_t v;
    char record_id[CT_MAX_ID_LEN];
} ct_bridge_t;

typedef struct {
    int32_t *articulation_nodes;
    size_t articulation_count;
    ct_bridge_t *bridges;
    size_t bridge_count;
} ct_tarjan_result_t;

ct_status_t ct_tarjan_analyze(const ct_graph_t *g, ct_tarjan_result_t *out_result);
void ct_tarjan_result_free(ct_tarjan_result_t *res);

#endif /* CHRONOTRACE_TARJAN_H */
