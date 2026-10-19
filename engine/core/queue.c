#include "queue.h"
#include <stdlib.h>

ct_status_t ct_queue_init(ct_queue_t *q, size_t initial_capacity) {
    if (!q) return CT_ERR_INVARG;
    if (initial_capacity == 0) initial_capacity = 16;

    q->data = (int32_t*)malloc(initial_capacity * sizeof(int32_t));
    if (!q->data) return CT_ERR_NOMEM;

    q->capacity = initial_capacity;
    q->head = 0;
    q->tail = 0;
    q->count = 0;
    return CT_OK;
}

static ct_status_t ct_queue_grow(ct_queue_t *q) {
    size_t new_cap = q->capacity * 2;
    int32_t *new_data = (int32_t*)malloc(new_cap * sizeof(int32_t));
    if (!new_data) return CT_ERR_NOMEM;

    for (size_t i = 0; i < q->count; i++) {
        new_data[i] = q->data[(q->head + i) % q->capacity];
    }

    free(q->data);
    q->data = new_data;
    q->head = 0;
    q->tail = q->count;
    q->capacity = new_cap;
    return CT_OK;
}

ct_status_t ct_queue_enqueue(ct_queue_t *q, int32_t value) {
    if (!q) return CT_ERR_INVARG;
    if (q->count == q->capacity) {
        ct_status_t status = ct_queue_grow(q);
        if (status != CT_OK) return status;
    }

    q->data[q->tail] = value;
    q->tail = (q->tail + 1) % q->capacity;
    q->count++;
    return CT_OK;
}

ct_status_t ct_queue_dequeue(ct_queue_t *q, int32_t *out_value) {
    if (!q) return CT_ERR_INVARG;
    if (q->count == 0) return CT_ERR_EMPTY;

    if (out_value) {
        *out_value = q->data[q->head];
    }
    q->head = (q->head + 1) % q->capacity;
    q->count--;
    return CT_OK;
}

ct_status_t ct_queue_peek(const ct_queue_t *q, int32_t *out_value) {
    if (!q || !out_value) return CT_ERR_INVARG;
    if (q->count == 0) return CT_ERR_EMPTY;
    *out_value = q->data[q->head];
    return CT_OK;
}

bool ct_queue_is_empty(const ct_queue_t *q) {
    return !q || q->count == 0;
}

size_t ct_queue_size(const ct_queue_t *q) {
    return q ? q->count : 0;
}

void ct_queue_clear(ct_queue_t *q) {
    if (q) {
        q->head = 0;
        q->tail = 0;
        q->count = 0;
    }
}

void ct_queue_free(ct_queue_t *q) {
    if (q) {
        if (q->data) {
            free(q->data);
            q->data = NULL;
        }
        q->capacity = 0;
        q->head = 0;
        q->tail = 0;
        q->count = 0;
    }
}
