#ifndef CHRONOTRACE_DSU_H
#define CHRONOTRACE_DSU_H

#include "../../include/common.h"

typedef struct {
    int32_t *parent;
    uint32_t *rank;
    uint32_t *size;
    size_t count;
    size_t num_components;
} ct_dsu_t;

ct_status_t ct_dsu_init(ct_dsu_t *dsu, size_t count);
int32_t ct_dsu_find(ct_dsu_t *dsu, int32_t x);
bool ct_dsu_union(ct_dsu_t *dsu, int32_t x, int32_t y);
bool ct_dsu_connected(ct_dsu_t *dsu, int32_t x, int32_t y);
uint32_t ct_dsu_group_size(ct_dsu_t *dsu, int32_t x);
void ct_dsu_free(ct_dsu_t *dsu);

#endif /* CHRONOTRACE_DSU_H */
