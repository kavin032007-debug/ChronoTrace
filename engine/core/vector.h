#ifndef CHRONOTRACE_VECTOR_H
#define CHRONOTRACE_VECTOR_H

#include "../../include/common.h"

typedef struct {
    void *data;
    size_t element_size;
    size_t size;
    size_t capacity;
} ct_vector_t;

ct_status_t ct_vector_init(ct_vector_t *vec, size_t element_size, size_t initial_capacity);
ct_status_t ct_vector_push(ct_vector_t *vec, const void *element);
void* ct_vector_get(const ct_vector_t *vec, size_t index);
void* ct_vector_last(const ct_vector_t *vec);
ct_status_t ct_vector_pop(ct_vector_t *vec, void *out_element);
ct_status_t ct_vector_set(ct_vector_t *vec, size_t index, const void *element);
ct_status_t ct_vector_reserve(ct_vector_t *vec, size_t min_capacity);
void ct_vector_clear(ct_vector_t *vec);
void ct_vector_free(ct_vector_t *vec);

#endif /* CHRONOTRACE_VECTOR_H */
