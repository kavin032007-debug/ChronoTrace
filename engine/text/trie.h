#ifndef CHRONOTRACE_TRIE_H
#define CHRONOTRACE_TRIE_H

#include "../../include/common.h"

typedef struct {
    char id[CT_MAX_ID_LEN];
    char name[CT_MAX_NAME_LEN];
    char type[CT_MAX_TYPE_LEN];
    int32_t record_index;
} ct_suggestion_t;

typedef struct ct_trie_node ct_trie_node_t;

struct ct_trie_node {
    char character;
    bool is_terminal;
    ct_suggestion_t data;
    ct_trie_node_t *first_child;
    ct_trie_node_t *next_sibling;
};

typedef struct {
    ct_trie_node_t *root;
    size_t total_words;
} ct_trie_t;

ct_status_t ct_trie_init(ct_trie_t *trie);
ct_status_t ct_trie_insert(ct_trie_t *trie, const char *key, const char *id,
                           const char *name, const char *type, int32_t record_index);
ct_status_t ct_trie_search_prefix(const ct_trie_t *trie, const char *prefix,
                                  size_t max_results,
                                  ct_suggestion_t **out_suggestions,
                                  size_t *out_count);
void ct_suggestions_free(ct_suggestion_t *suggestions);
void ct_trie_clear(ct_trie_t *trie);
void ct_trie_free(ct_trie_t *trie);

#endif /* CHRONOTRACE_TRIE_H */
