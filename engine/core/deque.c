#include "deque.h"
#include <stdlib.h>

ct_status_t ct_deque_init(ct_deque_t *dq, size_t initial_capacity) {
    if (!dq) return CT_ERR_INVARG;
    if (initial_capacity == 0) initial_capacity = 16;

    dq->data = (int32_t*)malloc(initial_capacity * sizeof(int32_t));
    if (!dq->data) return CT_ERR_NOMEM;

    dq->capacity = initial_capacity;
    dq->head = 0;
    dq->tail = 0;
    dq->count = 0;
    return CT_OK;
}

static ct_status_t ct_deque_grow(ct_deque_t *dq) {
    size_t new_cap = dq->capacity * 2;
    int32_t *new_data = (int32_t*)malloc(new_cap * sizeof(int32_t));
    if (!new_data) return CT_ERR_NOMEM;

    for (size_t i = 0; i < dq->count; i++) {
        new_data[i] = dq->data[(dq->head + i) % dq->capacity];
    }

    free(dq->data);
    dq->data = new_data;
    dq->head = 0;
    dq->tail = dq->count;
    dq->capacity = new_cap;
    return CT_OK;
}

ct_status_t ct_deque_push_back(ct_deque_t *dq, int32_t value) {
    if (!dq) return CT_ERR_INVARG;
    if (dq->count == dq->capacity) {
        ct_status_t status = ct_deque_grow(dq);
        if (status != CT_OK) return status;
    }

    dq->data[dq->tail] = value;
    dq->tail = (dq->tail + 1) % dq->capacity;
    dq->count++;
    return CT_OK;
}

ct_status_t ct_deque_push_front(ct_deque_t *dq, int32_t value) {
    if (!dq) return CT_ERR_INVARG;
    if (dq->count == dq->capacity) {
        ct_status_t status = ct_deque_grow(dq);
        if (status != CT_OK) return status;
    }

    dq->head = (dq->head == 0) ? (dq->capacity - 1) : (dq->head - 1);
    dq->data[dq->head] = value;
    dq->count++;
    return CT_OK;
}

ct_status_t ct_deque_pop_front(ct_deque_t *dq, int32_t *out_value) {
    if (!dq) return CT_ERR_INVARG;
    if (dq->count == 0) return CT_ERR_EMPTY;

    if (out_value) {
        *out_value = dq->data[dq->head];
    }
    dq->head = (dq->head + 1) % dq->capacity;
    dq->count--;
    return CT_OK;
}

ct_status_t ct_deque_pop_back(ct_deque_t *dq, int32_t *out_value) {
    if (!dq) return CT_ERR_INVARG;
    if (dq->count == 0) return CT_ERR_EMPTY;

    dq->tail = (dq->tail == 0) ? (dq->capacity - 1) : (dq->tail - 1);
    if (out_value) {
        *out_value = dq->data[dq->tail];
    }
    dq->count--;
    return CT_OK;
}

ct_status_t ct_deque_peek_front(const ct_deque_t *dq, int32_t *out_value) {
    if (!dq || !out_value) return CT_ERR_INVARG;
    if (dq->count == 0) return CT_ERR_EMPTY;
    *out_value = dq->data[dq->head];
    return CT_OK;
}

ct_status_t ct_deque_peek_back(const ct_deque_t *dq, int32_t *out_value) {
    if (!dq || !out_value) return CT_ERR_INVARG;
    if (dq->count == 0) return CT_ERR_EMPTY;
    size_t idx = (dq->tail == 0) ? (dq->capacity - 1) : (dq->tail - 1);
    *out_value = dq->data[idx];
    return CT_OK;
}

bool ct_deque_is_empty(const ct_deque_t *dq) {
    return !dq || dq->count == 0;
}

size_t ct_deque_size(const ct_deque_t *dq) {
    return dq ? dq->count : 0;
}

void ct_deque_clear(ct_deque_t *dq) {
    if (dq) {
        dq->head = 0;
        dq->tail = 0;
        dq->count = 0;
    }
}

void ct_deque_free(ct_deque_t *dq) {
    if (dq) {
        if (dq->data) {
            free(dq->data);
            dq->data = NULL;
        }
        dq->capacity = 0;
        dq->head = 0;
        dq->tail = 0;
        dq->count = 0;
    }
}
