#include "arena.h"
#include <stdlib.h>
#include <string.h>

#define CT_ARENA_ALIGNMENT 8
#define CT_ALIGN_UP(size) (((size) + (CT_ARENA_ALIGNMENT - 1)) & ~(CT_ARENA_ALIGNMENT - 1))

ct_status_t ct_arena_init(ct_arena_t *arena, size_t default_block_size) {
    if (!arena) return CT_ERR_INVARG;
    if (default_block_size < 1024) default_block_size = 64 * 1024; /* Default 64KB */

    arena->default_block_size = default_block_size;
    arena->total_allocated = 0;

    ct_arena_block_t *first = (ct_arena_block_t*)malloc(sizeof(ct_arena_block_t) + default_block_size);
    if (!first) return CT_ERR_NOMEM;

    first->capacity = default_block_size;
    first->used = 0;
    first->next = NULL;

    arena->head = first;
    arena->current = first;
    arena->total_allocated = sizeof(ct_arena_block_t) + default_block_size;

    return CT_OK;
}

void* ct_arena_alloc(ct_arena_t *arena, size_t size) {
    if (!arena || size == 0) return NULL;

    size_t aligned_size = CT_ALIGN_UP(size);
    ct_arena_block_t *cur = arena->current;

    if (cur && (cur->capacity - cur->used) >= aligned_size) {
        void *ptr = cur->data + cur->used;
        cur->used += aligned_size;
        return ptr;
    }

    /* Allocate new block */
    size_t block_size = arena->default_block_size;
    if (aligned_size > block_size) {
        block_size = aligned_size;
    }

    ct_arena_block_t *new_block = (ct_arena_block_t*)malloc(sizeof(ct_arena_block_t) + block_size);
    if (!new_block) return NULL;

    new_block->capacity = block_size;
    new_block->used = aligned_size;
    new_block->next = NULL;

    if (cur) {
        cur->next = new_block;
    } else {
        arena->head = new_block;
    }
    arena->current = new_block;
    arena->total_allocated += sizeof(ct_arena_block_t) + block_size;

    return new_block->data;
}

char* ct_arena_strdup(ct_arena_t *arena, const char *str) {
    if (!arena || !str) return NULL;
    size_t len = strlen(str) + 1;
    char *copy = (char*)ct_arena_alloc(arena, len);
    if (copy) {
        memcpy(copy, str, len);
    }
    return copy;
}

void ct_arena_reset(ct_arena_t *arena) {
    if (!arena || !arena->head) return;
    ct_arena_block_t *b = arena->head;
    while (b) {
        b->used = 0;
        b = b->next;
    }
    arena->current = arena->head;
}

void ct_arena_free(ct_arena_t *arena) {
    if (!arena) return;
    ct_arena_block_t *b = arena->head;
    while (b) {
        ct_arena_block_t *next = b->next;
        free(b);
        b = next;
    }
    arena->head = NULL;
    arena->current = NULL;
    arena->total_allocated = 0;
}
