#include "minheap.h"
#include <stdlib.h>
#include <string.h>

static int default_cmp(const ct_heap_node_t *a, const ct_heap_node_t *b) {
    if (a->priority < b->priority) return -1;
    if (a->priority > b->priority) return 1;
    if (a->id < b->id) return -1;
    if (a->id > b->id) return 1;
    return 0;
}

ct_status_t ct_minheap_init(ct_minheap_t *heap, size_t initial_capacity, size_t max_id_support, ct_heap_cmp_fn cmp) {
    if (!heap) return CT_ERR_INVARG;
    if (initial_capacity == 0) initial_capacity = 16;

    heap->nodes = (ct_heap_node_t*)malloc(initial_capacity * sizeof(ct_heap_node_t));
    if (!heap->nodes) return CT_ERR_NOMEM;

    heap->pos = NULL;
    heap->pos_capacity = 0;
    if (max_id_support > 0) {
        heap->pos = (int32_t*)malloc(max_id_support * sizeof(int32_t));
        if (!heap->pos) {
            free(heap->nodes);
            return CT_ERR_NOMEM;
        }
        for (size_t i = 0; i < max_id_support; i++) {
            heap->pos[i] = -1;
        }
        heap->pos_capacity = max_id_support;
    }

    heap->size = 0;
    heap->capacity = initial_capacity;
    heap->cmp = cmp ? cmp : default_cmp;
    return CT_OK;
}

static void ct_minheap_swap(ct_minheap_t *heap, size_t i, size_t j) {
    ct_heap_node_t tmp = heap->nodes[i];
    heap->nodes[i] = heap->nodes[j];
    heap->nodes[j] = tmp;

    if (heap->pos) {
        if (heap->nodes[i].id >= 0 && (size_t)heap->nodes[i].id < heap->pos_capacity) {
            heap->pos[heap->nodes[i].id] = (int32_t)i;
        }
        if (heap->nodes[j].id >= 0 && (size_t)heap->nodes[j].id < heap->pos_capacity) {
            heap->pos[heap->nodes[j].id] = (int32_t)j;
        }
    }
}

static void ct_minheap_sift_up(ct_minheap_t *heap, size_t idx) {
    while (idx > 0) {
        size_t parent = (idx - 1) / 2;
        if (heap->cmp(&heap->nodes[idx], &heap->nodes[parent]) < 0) {
            ct_minheap_swap(heap, idx, parent);
            idx = parent;
        } else {
            break;
        }
    }
}

static void ct_minheap_sift_down(ct_minheap_t *heap, size_t idx) {
    while (true) {
        size_t left = 2 * idx + 1;
        size_t right = 2 * idx + 2;
        size_t smallest = idx;

        if (left < heap->size && heap->cmp(&heap->nodes[left], &heap->nodes[smallest]) < 0) {
            smallest = left;
        }
        if (right < heap->size && heap->cmp(&heap->nodes[right], &heap->nodes[smallest]) < 0) {
            smallest = right;
        }

        if (smallest != idx) {
            ct_minheap_swap(heap, idx, smallest);
            idx = smallest;
        } else {
            break;
        }
    }
}

ct_status_t ct_minheap_insert(ct_minheap_t *heap, int32_t id, double priority, void *user_data) {
    if (!heap) return CT_ERR_INVARG;

    if (heap->pos && id >= 0 && (size_t)id >= heap->pos_capacity) {
        size_t new_pos_cap = (size_t)id * 2 + 16;
        int32_t *new_pos = (int32_t*)realloc(heap->pos, new_pos_cap * sizeof(int32_t));
        if (!new_pos) return CT_ERR_NOMEM;
        for (size_t i = heap->pos_capacity; i < new_pos_cap; i++) {
            new_pos[i] = -1;
        }
        heap->pos = new_pos;
        heap->pos_capacity = new_pos_cap;
    }

    if (heap->size >= heap->capacity) {
        size_t new_cap = heap->capacity * 2;
        ct_heap_node_t *new_nodes = (ct_heap_node_t*)realloc(heap->nodes, new_cap * sizeof(ct_heap_node_t));
        if (!new_nodes) return CT_ERR_NOMEM;
        heap->nodes = new_nodes;
        heap->capacity = new_cap;
    }

    size_t idx = heap->size++;
    heap->nodes[idx].id = id;
    heap->nodes[idx].priority = priority;
    heap->nodes[idx].user_data = user_data;

    if (heap->pos && id >= 0 && (size_t)id < heap->pos_capacity) {
        heap->pos[id] = (int32_t)idx;
    }

    ct_minheap_sift_up(heap, idx);
    return CT_OK;
}

ct_status_t ct_minheap_extract_min(ct_minheap_t *heap, ct_heap_node_t *out_node) {
    if (!heap) return CT_ERR_INVARG;
    if (heap->size == 0) return CT_ERR_EMPTY;

    if (out_node) {
        *out_node = heap->nodes[0];
    }

    if (heap->pos && heap->nodes[0].id >= 0 && (size_t)heap->nodes[0].id < heap->pos_capacity) {
        heap->pos[heap->nodes[0].id] = -1;
    }

    heap->size--;
    if (heap->size > 0) {
        heap->nodes[0] = heap->nodes[heap->size];
        if (heap->pos && heap->nodes[0].id >= 0 && (size_t)heap->nodes[0].id < heap->pos_capacity) {
            heap->pos[heap->nodes[0].id] = 0;
        }
        ct_minheap_sift_down(heap, 0);
    }

    return CT_OK;
}

ct_status_t ct_minheap_peek(const ct_minheap_t *heap, ct_heap_node_t *out_node) {
    if (!heap || !out_node) return CT_ERR_INVARG;
    if (heap->size == 0) return CT_ERR_EMPTY;
    *out_node = heap->nodes[0];
    return CT_OK;
}

ct_status_t ct_minheap_decrease_key(ct_minheap_t *heap, int32_t id, double new_priority) {
    if (!heap || !heap->pos || id < 0 || (size_t)id >= heap->pos_capacity) return CT_ERR_INVARG;

    int32_t idx = heap->pos[id];
    if (idx < 0 || (size_t)idx >= heap->size) return CT_ERR_NOTFOUND;

    double old_prio = heap->nodes[idx].priority;
    heap->nodes[idx].priority = new_priority;

    if (new_priority < old_prio) {
        ct_minheap_sift_up(heap, (size_t)idx);
    } else {
        ct_minheap_sift_down(heap, (size_t)idx);
    }
    return CT_OK;
}

bool ct_minheap_contains(const ct_minheap_t *heap, int32_t id) {
    if (!heap || !heap->pos || id < 0 || (size_t)id >= heap->pos_capacity) return false;
    return heap->pos[id] != -1;
}

bool ct_minheap_is_empty(const ct_minheap_t *heap) {
    return !heap || heap->size == 0;
}

size_t ct_minheap_size(const ct_minheap_t *heap) {
    return heap ? heap->size : 0;
}

void ct_minheap_clear(ct_minheap_t *heap) {
    if (!heap) return;
    if (heap->pos) {
        for (size_t i = 0; i < heap->pos_capacity; i++) {
            heap->pos[i] = -1;
        }
    }
    heap->size = 0;
}

void ct_minheap_free(ct_minheap_t *heap) {
    if (!heap) return;
    if (heap->nodes) {
        free(heap->nodes);
        heap->nodes = NULL;
    }
    if (heap->pos) {
        free(heap->pos);
        heap->pos = NULL;
    }
    heap->size = 0;
    heap->capacity = 0;
    heap->pos_capacity = 0;
}
