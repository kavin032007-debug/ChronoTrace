#ifndef CHRONOTRACE_STACK_H
#define CHRONOTRACE_STACK_H

#include "../../include/common.h"

typedef struct {
    int32_t *data;
    size_t capacity;
    size_t top;
} ct_stack_t;

ct_status_t ct_stack_init(ct_stack_t *s, size_t initial_capacity);
ct_status_t ct_stack_push(ct_stack_t *s, int32_t value);
ct_status_t ct_stack_pop(ct_stack_t *s, int32_t *out_value);
ct_status_t ct_stack_peek(const ct_stack_t *s, int32_t *out_value);
bool ct_stack_is_empty(const ct_stack_t *s);
size_t ct_stack_size(const ct_stack_t *s);
void ct_stack_clear(ct_stack_t *s);
void ct_stack_free(ct_stack_t *s);

#endif /* CHRONOTRACE_STACK_H */
