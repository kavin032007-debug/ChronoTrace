#include "hashmap.h"
#include <stdlib.h>
#include <string.h>

#define CT_HASHMAP_DEFAULT_BUCKETS 16
#define CT_HASHMAP_LOAD_FACTOR_NUM 3
#define CT_HASHMAP_LOAD_FACTOR_DEN 4

static uint32_t ct_murmur3_32(const char *key, size_t len, uint32_t seed) {
    uint32_t h = seed;
    const uint8_t *data = (const uint8_t*)key;
    size_t nblocks = len / 4;

    for (size_t i = 0; i < nblocks; i++) {
        uint32_t k = (uint32_t)data[i * 4] |
                     ((uint32_t)data[i * 4 + 1] << 8) |
                     ((uint32_t)data[i * 4 + 2] << 16) |
                     ((uint32_t)data[i * 4 + 3] << 24);

        k *= 0xcc9e2d51;
        k = (k << 15) | (k >> (32 - 15));
        k *= 0x1b873593;

        h ^= k;
        h = (h << 13) | (h >> (32 - 13));
        h = h * 5 + 0xe6546b64;
    }

    const uint8_t *tail = data + (nblocks * 4);
    uint32_t k1 = 0;
    switch (len & 3) {
        case 3: k1 ^= (uint32_t)tail[2] << 16; /* fallthrough */
        case 2: k1 ^= (uint32_t)tail[1] << 8;  /* fallthrough */
        case 1: k1 ^= (uint32_t)tail[0];
                k1 *= 0xcc9e2d51;
                k1 = (k1 << 15) | (k1 >> (32 - 15));
                k1 *= 0x1b873593;
                h ^= k1;
    }

    h ^= (uint32_t)len;
    h ^= h >> 16;
    h *= 0x85ebca6b;
    h ^= h >> 13;
    h *= 0xc2b2ae35;
    h ^= h >> 16;
    return h;
}

static size_t ct_next_pow2(size_t v) {
    if (v < 8) return 8;
    v--;
    v |= v >> 1;
    v |= v >> 2;
    v |= v >> 4;
    v |= v >> 8;
    v |= v >> 16;
#if SIZE_MAX > 0xFFFFFFFF
    v |= v >> 32;
#endif
    v++;
    return v;
}

ct_status_t ct_hashmap_init(ct_hashmap_t *map, size_t initial_buckets) {
    if (!map) return CT_ERR_INVARG;
    size_t buckets = ct_next_pow2(initial_buckets ? initial_buckets : CT_HASHMAP_DEFAULT_BUCKETS);

    map->buckets = (ct_hash_entry_t**)calloc(buckets, sizeof(ct_hash_entry_t*));
    if (!map->buckets) return CT_ERR_NOMEM;

    map->bucket_count = buckets;
    map->size = 0;
    map->seed = 0x9747b28c;
    return CT_OK;
}

static ct_status_t ct_hashmap_rehash(ct_hashmap_t *map) {
    size_t new_cap = map->bucket_count * 2;
    ct_hash_entry_t **new_buckets = (ct_hash_entry_t**)calloc(new_cap, sizeof(ct_hash_entry_t*));
    if (!new_buckets) return CT_ERR_NOMEM;

    for (size_t i = 0; i < map->bucket_count; i++) {
        ct_hash_entry_t *entry = map->buckets[i];
        while (entry) {
            ct_hash_entry_t *next = entry->next;
            uint32_t hash = ct_murmur3_32(entry->key, strlen(entry->key), map->seed);
            size_t idx = hash & (new_cap - 1);
            entry->next = new_buckets[idx];
            new_buckets[idx] = entry;
            entry = next;
        }
    }

    free(map->buckets);
    map->buckets = new_buckets;
    map->bucket_count = new_cap;
    return CT_OK;
}

ct_status_t ct_hashmap_put(ct_hashmap_t *map, const char *key, void *value, bool allow_overwrite) {
    if (!map || !key) return CT_ERR_INVARG;

    /* Check load factor (size + 1) / bucket_count > 0.75 */
    if ((map->size + 1) * CT_HASHMAP_LOAD_FACTOR_DEN > map->bucket_count * CT_HASHMAP_LOAD_FACTOR_NUM) {
        ct_status_t st = ct_hashmap_rehash(map);
        if (st != CT_OK) return st;
    }

    uint32_t hash = ct_murmur3_32(key, strlen(key), map->seed);
    size_t idx = hash & (map->bucket_count - 1);

    ct_hash_entry_t *cur = map->buckets[idx];
    while (cur) {
        if (strcmp(cur->key, key) == 0) {
            if (!allow_overwrite) {
                return CT_ERR_DUPLICATE;
            }
            cur->value = value;
            return CT_OK;
        }
        cur = cur->next;
    }

    ct_hash_entry_t *new_entry = (ct_hash_entry_t*)malloc(sizeof(ct_hash_entry_t));
    if (!new_entry) return CT_ERR_NOMEM;

    new_entry->key = (char*)malloc(strlen(key) + 1);
    if (!new_entry->key) {
        free(new_entry);
        return CT_ERR_NOMEM;
    }
    strcpy(new_entry->key, key);
    new_entry->value = value;
    new_entry->next = map->buckets[idx];
    map->buckets[idx] = new_entry;
    map->size++;

    return CT_OK;
}

ct_status_t ct_hashmap_get(const ct_hashmap_t *map, const char *key, void **out_value) {
    if (!map || !key) return CT_ERR_INVARG;

    uint32_t hash = ct_murmur3_32(key, strlen(key), map->seed);
    size_t idx = hash & (map->bucket_count - 1);

    ct_hash_entry_t *cur = map->buckets[idx];
    while (cur) {
        if (strcmp(cur->key, key) == 0) {
            if (out_value) *out_value = cur->value;
            return CT_OK;
        }
        cur = cur->next;
    }

    return CT_ERR_NOTFOUND;
}

bool ct_hashmap_contains(const ct_hashmap_t *map, const char *key) {
    return ct_hashmap_get(map, key, NULL) == CT_OK;
}

ct_status_t ct_hashmap_remove(ct_hashmap_t *map, const char *key, void **out_old_value) {
    if (!map || !key) return CT_ERR_INVARG;

    uint32_t hash = ct_murmur3_32(key, strlen(key), map->seed);
    size_t idx = hash & (map->bucket_count - 1);

    ct_hash_entry_t *cur = map->buckets[idx];
    ct_hash_entry_t *prev = NULL;

    while (cur) {
        if (strcmp(cur->key, key) == 0) {
            if (prev) {
                prev->next = cur->next;
            } else {
                map->buckets[idx] = cur->next;
            }
            if (out_old_value) *out_old_value = cur->value;
            free(cur->key);
            free(cur);
            map->size--;
            return CT_OK;
        }
        prev = cur;
        cur = cur->next;
    }

    return CT_ERR_NOTFOUND;
}

size_t ct_hashmap_size(const ct_hashmap_t *map) {
    return map ? map->size : 0;
}

ct_status_t ct_hashmap_iterate(const ct_hashmap_t *map, bool (*callback)(const char *key, void *value, void *ctx), void *ctx) {
    if (!map || !callback) return CT_ERR_INVARG;

    for (size_t i = 0; i < map->bucket_count; i++) {
        ct_hash_entry_t *entry = map->buckets[i];
        while (entry) {
            if (!callback(entry->key, entry->value, ctx)) {
                return CT_OK;
            }
            entry = entry->next;
        }
    }
    return CT_OK;
}

void ct_hashmap_clear(ct_hashmap_t *map, void (*free_value_fn)(void *val)) {
    if (!map || !map->buckets) return;

    for (size_t i = 0; i < map->bucket_count; i++) {
        ct_hash_entry_t *entry = map->buckets[i];
        while (entry) {
            ct_hash_entry_t *next = entry->next;
            if (free_value_fn && entry->value) {
                free_value_fn(entry->value);
            }
            free(entry->key);
            free(entry);
            entry = next;
        }
        map->buckets[i] = NULL;
    }
    map->size = 0;
}

void ct_hashmap_free(ct_hashmap_t *map, void (*free_value_fn)(void *val)) {
    if (!map) return;
    ct_hashmap_clear(map, free_value_fn);
    if (map->buckets) {
        free(map->buckets);
        map->buckets = NULL;
    }
    map->bucket_count = 0;
}
