#ifndef CHRONOTRACE_DEQUE_H
#define CHRONOTRACE_DEQUE_H

#include "../../include/common.h"

typedef struct {
    int32_t *data;
    size_t capacity;
    size_t head;
    size_t tail;
    size_t count;
} ct_deque_t;

ct_status_t ct_deque_init(ct_deque_t *dq, size_t initial_capacity);
ct_status_t ct_deque_push_back(ct_deque_t *dq, int32_t value);
ct_status_t ct_deque_push_front(ct_deque_t *dq, int32_t value);
ct_status_t ct_deque_pop_front(ct_deque_t *dq, int32_t *out_value);
ct_status_t ct_deque_pop_back(ct_deque_t *dq, int32_t *out_value);
ct_status_t ct_deque_peek_front(const ct_deque_t *dq, int32_t *out_value);
ct_status_t ct_deque_peek_back(const ct_deque_t *dq, int32_t *out_value);
bool ct_deque_is_empty(const ct_deque_t *dq);
size_t ct_deque_size(const ct_deque_t *dq);
void ct_deque_clear(ct_deque_t *dq);
void ct_deque_free(ct_deque_t *dq);

#endif /* CHRONOTRACE_DEQUE_H */
