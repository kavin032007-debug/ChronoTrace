#ifndef CHRONOTRACE_QUEUE_H
#define CHRONOTRACE_QUEUE_H

#include "../../include/common.h"

typedef struct {
    int32_t *data;
    size_t capacity;
    size_t head;
    size_t tail;
    size_t count;
} ct_queue_t;

ct_status_t ct_queue_init(ct_queue_t *q, size_t initial_capacity);
ct_status_t ct_queue_enqueue(ct_queue_t *q, int32_t value);
ct_status_t ct_queue_dequeue(ct_queue_t *q, int32_t *out_value);
ct_status_t ct_queue_peek(const ct_queue_t *q, int32_t *out_value);
bool ct_queue_is_empty(const ct_queue_t *q);
size_t ct_queue_size(const ct_queue_t *q);
void ct_queue_clear(ct_queue_t *q);
void ct_queue_free(ct_queue_t *q);

#endif /* CHRONOTRACE_QUEUE_H */
