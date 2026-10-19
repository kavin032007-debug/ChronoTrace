#ifndef CHRONOTRACE_LRU_H
#define CHRONOTRACE_LRU_H

#include "../../include/common.h"
#include "hashmap.h"

typedef struct ct_lru_node {
    char *key;
    char *value;
    uint64_t version;
    struct ct_lru_node *prev;
    struct ct_lru_node *next;
} ct_lru_node_t;

typedef struct {
    ct_hashmap_t map;
    ct_lru_node_t *head; /* Most recently used */
    ct_lru_node_t *tail; /* Least recently used */
    size_t capacity;
    size_t size;
    uint64_t current_version;
} ct_lru_t;

ct_status_t ct_lru_init(ct_lru_t *cache, size_t capacity);
ct_status_t ct_lru_get(ct_lru_t *cache, const char *key, const char **out_value);
ct_status_t ct_lru_put(ct_lru_t *cache, const char *key, const char *value);
void ct_lru_set_version(ct_lru_t *cache, uint64_t version);
uint64_t ct_lru_get_version(const ct_lru_t *cache);
void ct_lru_invalidate(ct_lru_t *cache);
void ct_lru_free(ct_lru_t *cache);

#endif /* CHRONOTRACE_LRU_H */
