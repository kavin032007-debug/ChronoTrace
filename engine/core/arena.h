#ifndef CHRONOTRACE_ARENA_H
#define CHRONOTRACE_ARENA_H

#include "../../include/common.h"

typedef struct ct_arena_block ct_arena_block_t;

struct ct_arena_block {
    size_t capacity;
    size_t used;
    ct_arena_block_t *next;
    uint8_t data[];
};

typedef struct {
    ct_arena_block_t *head;
    ct_arena_block_t *current;
    size_t default_block_size;
    size_t total_allocated;
} ct_arena_t;

ct_status_t ct_arena_init(ct_arena_t *arena, size_t default_block_size);
void* ct_arena_alloc(ct_arena_t *arena, size_t size);
char* ct_arena_strdup(ct_arena_t *arena, const char *str);
void ct_arena_reset(ct_arena_t *arena);
void ct_arena_free(ct_arena_t *arena);

#endif /* CHRONOTRACE_ARENA_H */
