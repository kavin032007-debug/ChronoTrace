#include "lru.h"
#include <stdlib.h>
#include <string.h>

ct_status_t ct_lru_init(ct_lru_t *cache, size_t capacity) {
    if (!cache) return CT_ERR_INVARG;
    if (capacity == 0) capacity = 64;

    ct_status_t st = ct_hashmap_init(&cache->map, capacity * 2);
    if (st != CT_OK) return st;

    cache->head = NULL;
    cache->tail = NULL;
    cache->capacity = capacity;
    cache->size = 0;
    cache->current_version = 1;
    return CT_OK;
}

static void ct_lru_detach(ct_lru_t *cache, ct_lru_node_t *node) {
    if (node->prev) node->prev->next = node->next;
    else cache->head = node->next;

    if (node->next) node->next->prev = node->prev;
    else cache->tail = node->prev;

    node->prev = NULL;
    node->next = NULL;
}

static void ct_lru_attach_front(ct_lru_t *cache, ct_lru_node_t *node) {
    node->next = cache->head;
    node->prev = NULL;
    if (cache->head) cache->head->prev = node;
    cache->head = node;
    if (!cache->tail) cache->tail = node;
}

ct_status_t ct_lru_get(ct_lru_t *cache, const char *key, const char **out_value) {
    if (!cache || !key || !out_value) return CT_ERR_INVARG;

    void *val_ptr = NULL;
    ct_status_t st = ct_hashmap_get(&cache->map, key, &val_ptr);
    if (st != CT_OK) return CT_ERR_NOTFOUND;

    ct_lru_node_t *node = (ct_lru_node_t*)val_ptr;
    if (node->version != cache->current_version) {
        /* Stale entry from older version */
        ct_lru_detach(cache, node);
        ct_hashmap_remove(&cache->map, key, NULL);
        free(node->key);
        free(node->value);
        free(node);
        cache->size--;
        return CT_ERR_NOTFOUND;
    }

    /* Move to front */
    if (cache->head != node) {
        ct_lru_detach(cache, node);
        ct_lru_attach_front(cache, node);
    }

    *out_value = node->value;
    return CT_OK;
}

ct_status_t ct_lru_put(ct_lru_t *cache, const char *key, const char *value) {
    if (!cache || !key || !value) return CT_ERR_INVARG;

    void *val_ptr = NULL;
    if (ct_hashmap_get(&cache->map, key, &val_ptr) == CT_OK) {
        ct_lru_node_t *node = (ct_lru_node_t*)val_ptr;
        free(node->value);
        node->value = (char*)malloc(strlen(value) + 1);
        if (!node->value) return CT_ERR_NOMEM;
        strcpy(node->value, value);
        node->version = cache->current_version;

        if (cache->head != node) {
            ct_lru_detach(cache, node);
            ct_lru_attach_front(cache, node);
        }
        return CT_OK;
    }

    /* Evict tail if capacity reached */
    if (cache->size >= cache->capacity && cache->tail) {
        ct_lru_node_t *evicted = cache->tail;
        ct_lru_detach(cache, evicted);
        ct_hashmap_remove(&cache->map, evicted->key, NULL);
        free(evicted->key);
        free(evicted->value);
        free(evicted);
        cache->size--;
    }

    ct_lru_node_t *new_node = (ct_lru_node_t*)malloc(sizeof(ct_lru_node_t));
    if (!new_node) return CT_ERR_NOMEM;

    new_node->key = (char*)malloc(strlen(key) + 1);
    new_node->value = (char*)malloc(strlen(value) + 1);
    if (!new_node->key || !new_node->value) {
        free(new_node->key);
        free(new_node->value);
        free(new_node);
        return CT_ERR_NOMEM;
    }

    strcpy(new_node->key, key);
    strcpy(new_node->value, value);
    new_node->version = cache->current_version;
    new_node->prev = NULL;
    new_node->next = NULL;

    ct_status_t st = ct_hashmap_put(&cache->map, key, new_node, true);
    if (st != CT_OK) {
        free(new_node->key);
        free(new_node->value);
        free(new_node);
        return st;
    }

    ct_lru_attach_front(cache, new_node);
    cache->size++;
    return CT_OK;
}

void ct_lru_set_version(ct_lru_t *cache, uint64_t version) {
    if (!cache) return;
    cache->current_version = version;
    ct_lru_invalidate(cache);
}

uint64_t ct_lru_get_version(const ct_lru_t *cache) {
    return cache ? cache->current_version : 0;
}

void ct_lru_invalidate(ct_lru_t *cache) {
    if (!cache) return;
    ct_lru_node_t *cur = cache->head;
    while (cur) {
        ct_lru_node_t *next = cur->next;
        free(cur->key);
        free(cur->value);
        free(cur);
        cur = next;
    }
    cache->head = NULL;
    cache->tail = NULL;
    cache->size = 0;
    ct_hashmap_clear(&cache->map, NULL);
}

void ct_lru_free(ct_lru_t *cache) {
    if (!cache) return;
    ct_lru_invalidate(cache);
    ct_hashmap_free(&cache->map, NULL);
}
