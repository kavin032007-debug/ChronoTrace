#include "stack.h"
#include <stdlib.h>

ct_status_t ct_stack_init(ct_stack_t *s, size_t initial_capacity) {
    if (!s) return CT_ERR_INVARG;
    if (initial_capacity == 0) initial_capacity = 16;

    s->data = (int32_t*)malloc(initial_capacity * sizeof(int32_t));
    if (!s->data) return CT_ERR_NOMEM;

    s->capacity = initial_capacity;
    s->top = 0;
    return CT_OK;
}

ct_status_t ct_stack_push(ct_stack_t *s, int32_t value) {
    if (!s) return CT_ERR_INVARG;
    if (s->top >= s->capacity) {
        size_t new_cap = s->capacity * 2;
        int32_t *new_data = (int32_t*)realloc(s->data, new_cap * sizeof(int32_t));
        if (!new_data) return CT_ERR_NOMEM;
        s->data = new_data;
        s->capacity = new_cap;
    }

    s->data[s->top++] = value;
    return CT_OK;
}

ct_status_t ct_stack_pop(ct_stack_t *s, int32_t *out_value) {
    if (!s) return CT_ERR_INVARG;
    if (s->top == 0) return CT_ERR_EMPTY;

    s->top--;
    if (out_value) {
        *out_value = s->data[s->top];
    }
    return CT_OK;
}

ct_status_t ct_stack_peek(const ct_stack_t *s, int32_t *out_value) {
    if (!s || !out_value) return CT_ERR_INVARG;
    if (s->top == 0) return CT_ERR_EMPTY;
    *out_value = s->data[s->top - 1];
    return CT_OK;
}

bool ct_stack_is_empty(const ct_stack_t *s) {
    return !s || s->top == 0;
}

size_t ct_stack_size(const ct_stack_t *s) {
    return s ? s->top : 0;
}

void ct_stack_clear(ct_stack_t *s) {
    if (s) {
        s->top = 0;
    }
}

void ct_stack_free(ct_stack_t *s) {
    if (s) {
        if (s->data) {
            free(s->data);
            s->data = NULL;
        }
        s->capacity = 0;
        s->top = 0;
    }
}
