#ifndef CHRONOTRACE_HASHMAP_H
#define CHRONOTRACE_HASHMAP_H

#include "../../include/common.h"

typedef struct ct_hash_entry {
    char *key;
    void *value;
    struct ct_hash_entry *next;
} ct_hash_entry_t;

typedef struct {
    ct_hash_entry_t **buckets;
    size_t bucket_count;
    size_t size;
    uint32_t seed;
} ct_hashmap_t;

ct_status_t ct_hashmap_init(ct_hashmap_t *map, size_t initial_buckets);
ct_status_t ct_hashmap_put(ct_hashmap_t *map, const char *key, void *value, bool allow_overwrite);
ct_status_t ct_hashmap_get(const ct_hashmap_t *map, const char *key, void **out_value);
bool ct_hashmap_contains(const ct_hashmap_t *map, const char *key);
ct_status_t ct_hashmap_remove(ct_hashmap_t *map, const char *key, void **out_old_value);
size_t ct_hashmap_size(const ct_hashmap_t *map);
ct_status_t ct_hashmap_iterate(const ct_hashmap_t *map, bool (*callback)(const char *key, void *value, void *ctx), void *ctx);
void ct_hashmap_clear(ct_hashmap_t *map, void (*free_value_fn)(void *val));
void ct_hashmap_free(ct_hashmap_t *map, void (*free_value_fn)(void *val));

#endif /* CHRONOTRACE_HASHMAP_H */
