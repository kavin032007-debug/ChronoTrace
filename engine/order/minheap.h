#ifndef CHRONOTRACE_MINHEAP_H
#define CHRONOTRACE_MINHEAP_H

#include "../../include/common.h"

typedef struct {
    int32_t id;          /* Entity / Node / Item ID integer identifier */
    double priority;     /* Priority key (e.g. distance or timestamp) */
    void *user_data;     /* Optional payload */
} ct_heap_node_t;

typedef int (*ct_heap_cmp_fn)(const ct_heap_node_t *a, const ct_heap_node_t *b);

typedef struct {
    ct_heap_node_t *nodes;
    int32_t *pos;        /* pos[id] = index in nodes array, or -1 if not in heap */
    size_t pos_capacity;
    size_t size;
    size_t capacity;
    ct_heap_cmp_fn cmp;
} ct_minheap_t;

ct_status_t ct_minheap_init(ct_minheap_t *heap, size_t initial_capacity, size_t max_id_support, ct_heap_cmp_fn cmp);
ct_status_t ct_minheap_insert(ct_minheap_t *heap, int32_t id, double priority, void *user_data);
ct_status_t ct_minheap_extract_min(ct_minheap_t *heap, ct_heap_node_t *out_node);
ct_status_t ct_minheap_peek(const ct_minheap_t *heap, ct_heap_node_t *out_node);
ct_status_t ct_minheap_decrease_key(ct_minheap_t *heap, int32_t id, double new_priority);
bool ct_minheap_contains(const ct_minheap_t *heap, int32_t id);
bool ct_minheap_is_empty(const ct_minheap_t *heap);
size_t ct_minheap_size(const ct_minheap_t *heap);
void ct_minheap_clear(ct_minheap_t *heap);
void ct_minheap_free(ct_minheap_t *heap);

#endif /* CHRONOTRACE_MINHEAP_H */
