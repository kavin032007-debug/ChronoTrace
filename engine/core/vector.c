#include "vector.h"
#include <stdlib.h>
#include <string.h>

ct_status_t ct_vector_init(ct_vector_t *vec, size_t element_size, size_t initial_capacity) {
    if (!vec || element_size == 0) return CT_ERR_INVARG;
    if (initial_capacity == 0) initial_capacity = 8;

    vec->data = malloc(element_size * initial_capacity);
    if (!vec->data) return CT_ERR_NOMEM;

    vec->element_size = element_size;
    vec->size = 0;
    vec->capacity = initial_capacity;
    return CT_OK;
}

ct_status_t ct_vector_reserve(ct_vector_t *vec, size_t min_capacity) {
    if (!vec) return CT_ERR_INVARG;
    if (min_capacity <= vec->capacity) return CT_OK;

    size_t new_cap = vec->capacity ? vec->capacity * 2 : 8;
    while (new_cap < min_capacity) {
        new_cap *= 2;
    }

    void *new_data = realloc(vec->data, new_cap * vec->element_size);
    if (!new_data) return CT_ERR_NOMEM;

    vec->data = new_data;
    vec->capacity = new_cap;
    return CT_OK;
}

ct_status_t ct_vector_push(ct_vector_t *vec, const void *element) {
    if (!vec) return CT_ERR_INVARG;
    if (vec->size >= vec->capacity) {
        ct_status_t status = ct_vector_reserve(vec, vec->capacity * 2);
        if (status != CT_OK) return status;
    }

    char *target = (char*)vec->data + (vec->size * vec->element_size);
    if (element) {
        memcpy(target, element, vec->element_size);
    } else {
        memset(target, 0, vec->element_size);
    }
    vec->size++;
    return CT_OK;
}

void* ct_vector_get(const ct_vector_t *vec, size_t index) {
    if (!vec || index >= vec->size) return NULL;
    return (char*)vec->data + (index * vec->element_size);
}

void* ct_vector_last(const ct_vector_t *vec) {
    if (!vec || vec->size == 0) return NULL;
    return (char*)vec->data + ((vec->size - 1) * vec->element_size);
}

ct_status_t ct_vector_pop(ct_vector_t *vec, void *out_element) {
    if (!vec || vec->size == 0) return CT_ERR_EMPTY;
    vec->size--;
    if (out_element) {
        memcpy(out_element, (char*)vec->data + (vec->size * vec->element_size), vec->element_size);
    }
    return CT_OK;
}

ct_status_t ct_vector_set(ct_vector_t *vec, size_t index, const void *element) {
    if (!vec || !element || index >= vec->size) return CT_ERR_INVARG;
    memcpy((char*)vec->data + (index * vec->element_size), element, vec->element_size);
    return CT_OK;
}

void ct_vector_clear(ct_vector_t *vec) {
    if (vec) {
        vec->size = 0;
    }
}

void ct_vector_free(ct_vector_t *vec) {
    if (vec) {
        if (vec->data) {
            free(vec->data);
            vec->data = NULL;
        }
        vec->size = 0;
        vec->capacity = 0;
        vec->element_size = 0;
    }
}
