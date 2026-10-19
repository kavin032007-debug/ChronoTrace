#include "trie.h"
#include <stdlib.h>
#include <string.h>
#include <ctype.h>

static ct_trie_node_t* ct_trie_create_node(char c) {
    ct_trie_node_t *node = (ct_trie_node_t*)malloc(sizeof(ct_trie_node_t));
    if (!node) return NULL;
    node->character = c;
    node->is_terminal = false;
    memset(&node->data, 0, sizeof(ct_suggestion_t));
    node->first_child = NULL;
    node->next_sibling = NULL;
    return node;
}

ct_status_t ct_trie_init(ct_trie_t *trie) {
    if (!trie) return CT_ERR_INVARG;
    trie->root = ct_trie_create_node('\0');
    if (!trie->root) return CT_ERR_NOMEM;
    trie->total_words = 0;
    return CT_OK;
}

static ct_trie_node_t* ct_trie_find_child(ct_trie_node_t *parent, char c) {
    ct_trie_node_t *child = parent->first_child;
    while (child) {
        if (child->character == c) return child;
        child = child->next_sibling;
    }
    return NULL;
}

static ct_trie_node_t* ct_trie_get_or_add_child(ct_trie_node_t *parent, char c) {
    ct_trie_node_t *child = ct_trie_find_child(parent, c);
    if (child) return child;

    ct_trie_node_t *new_node = ct_trie_create_node(c);
    if (!new_node) return NULL;

    new_node->next_sibling = parent->first_child;
    parent->first_child = new_node;
    return new_node;
}

ct_status_t ct_trie_insert(ct_trie_t *trie, const char *key, const char *id,
                           const char *name, const char *type, int32_t record_index) {
    if (!trie || !trie->root || !key || !id) return CT_ERR_INVARG;

    ct_trie_node_t *curr = trie->root;
    for (size_t i = 0; key[i] != '\0'; i++) {
        char c = (char)tolower((unsigned char)key[i]);
        curr = ct_trie_get_or_add_child(curr, c);
        if (!curr) return CT_ERR_NOMEM;
    }

    if (!curr->is_terminal) {
        curr->is_terminal = true;
        trie->total_words++;
    }

    strncpy(curr->data.id, id, CT_MAX_ID_LEN - 1);
    curr->data.id[CT_MAX_ID_LEN - 1] = '\0';
    strncpy(curr->data.name, name ? name : "", CT_MAX_NAME_LEN - 1);
    curr->data.name[CT_MAX_NAME_LEN - 1] = '\0';
    strncpy(curr->data.type, type ? type : "", CT_MAX_TYPE_LEN - 1);
    curr->data.type[CT_MAX_TYPE_LEN - 1] = '\0';
    curr->data.record_index = record_index;

    return CT_OK;
}

static void ct_trie_collect_dfs(ct_trie_node_t *node, ct_suggestion_t *results,
                                size_t max_results, size_t *count) {
    if (!node || *count >= max_results) return;

    if (node->is_terminal) {
        results[(*count)++] = node->data;
        if (*count >= max_results) return;
    }

    ct_trie_node_t *child = node->first_child;
    while (child && *count < max_results) {
        ct_trie_collect_dfs(child, results, max_results, count);
        child = child->next_sibling;
    }
}

ct_status_t ct_trie_search_prefix(const ct_trie_t *trie, const char *prefix,
                                  size_t max_results,
                                  ct_suggestion_t **out_suggestions,
                                  size_t *out_count) {
    if (!trie || !trie->root || !out_suggestions || !out_count) return CT_ERR_INVARG;
    *out_suggestions = NULL;
    *out_count = 0;

    if (max_results == 0) max_results = 10;

    ct_trie_node_t *curr = trie->root;
    if (prefix && prefix[0] != '\0') {
        for (size_t i = 0; prefix[i] != '\0'; i++) {
            char c = (char)tolower((unsigned char)prefix[i]);
            curr = ct_trie_find_child(curr, c);
            if (!curr) {
                return CT_OK; /* Prefix not found, return empty */
            }
        }
    }

    ct_suggestion_t *results = (ct_suggestion_t*)malloc(max_results * sizeof(ct_suggestion_t));
    if (!results) return CT_ERR_NOMEM;

    size_t count = 0;
    ct_trie_collect_dfs(curr, results, max_results, &count);

    *out_suggestions = results;
    *out_count = count;
    return CT_OK;
}

void ct_suggestions_free(ct_suggestion_t *suggestions) {
    if (suggestions) {
        free(suggestions);
    }
}

static void ct_trie_free_subtree(ct_trie_node_t *node) {
    if (!node) return;
    ct_trie_node_t *child = node->first_child;
    while (child) {
        ct_trie_node_t *next = child->next_sibling;
        ct_trie_free_subtree(child);
        child = next;
    }
    free(node);
}

void ct_trie_clear(ct_trie_t *trie) {
    if (!trie || !trie->root) return;
    ct_trie_node_t *child = trie->root->first_child;
    while (child) {
        ct_trie_node_t *next = child->next_sibling;
        ct_trie_free_subtree(child);
        child = next;
    }
    trie->root->first_child = NULL;
    trie->total_words = 0;
}

void ct_trie_free(ct_trie_t *trie) {
    if (!trie) return;
    ct_trie_free_subtree(trie->root);
    trie->root = NULL;
    trie->total_words = 0;
}
