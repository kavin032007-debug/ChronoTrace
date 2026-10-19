#include "dsu.h"
#include <stdlib.h>

ct_status_t ct_dsu_init(ct_dsu_t *dsu, size_t count) {
    if (!dsu) return CT_ERR_INVARG;

    dsu->parent = (int32_t*)malloc(count * sizeof(int32_t));
    dsu->rank = (uint32_t*)malloc(count * sizeof(uint32_t));
    dsu->size = (uint32_t*)malloc(count * sizeof(uint32_t));

    if (!dsu->parent || !dsu->rank || !dsu->size) {
        free(dsu->parent);
        free(dsu->rank);
        free(dsu->size);
        return CT_ERR_NOMEM;
    }

    for (size_t i = 0; i < count; i++) {
        dsu->parent[i] = (int32_t)i;
        dsu->rank[i] = 0;
        dsu->size[i] = 1;
    }

    dsu->count = count;
    dsu->num_components = count;
    return CT_OK;
}

int32_t ct_dsu_find(ct_dsu_t *dsu, int32_t x) {
    if (!dsu || x < 0 || (size_t)x >= dsu->count) return -1;

    int32_t root = x;
    while (root != dsu->parent[root]) {
        root = dsu->parent[root];
    }

    /* Path compression */
    int32_t curr = x;
    while (curr != root) {
        int32_t next = dsu->parent[curr];
        dsu->parent[curr] = root;
        curr = next;
    }
    return root;
}

bool ct_dsu_union(ct_dsu_t *dsu, int32_t x, int32_t y) {
    if (!dsu) return false;
    int32_t root_x = ct_dsu_find(dsu, x);
    int32_t root_y = ct_dsu_find(dsu, y);

    if (root_x == -1 || root_y == -1 || root_x == root_y) {
        return false;
    }

    /* Union by rank */
    if (dsu->rank[root_x] < dsu->rank[root_y]) {
        dsu->parent[root_x] = root_y;
        dsu->size[root_y] += dsu->size[root_x];
    } else if (dsu->rank[root_x] > dsu->rank[root_y]) {
        dsu->parent[root_y] = root_x;
        dsu->size[root_x] += dsu->size[root_y];
    } else {
        dsu->parent[root_y] = root_x;
        dsu->size[root_x] += dsu->size[root_y];
        dsu->rank[root_x]++;
    }

    dsu->num_components--;
    return true;
}

bool ct_dsu_connected(ct_dsu_t *dsu, int32_t x, int32_t y) {
    if (!dsu) return false;
    int32_t root_x = ct_dsu_find(dsu, x);
    int32_t root_y = ct_dsu_find(dsu, y);
    return (root_x != -1 && root_y != -1 && root_x == root_y);
}

uint32_t ct_dsu_group_size(ct_dsu_t *dsu, int32_t x) {
    if (!dsu) return 0;
    int32_t root = ct_dsu_find(dsu, x);
    if (root == -1) return 0;
    return dsu->size[root];
}

void ct_dsu_free(ct_dsu_t *dsu) {
    if (!dsu) return;
    if (dsu->parent) free(dsu->parent);
    if (dsu->rank) free(dsu->rank);
    if (dsu->size) free(dsu->size);
    dsu->parent = NULL;
    dsu->rank = NULL;
    dsu->size = NULL;
    dsu->count = 0;
    dsu->num_components = 0;
}
