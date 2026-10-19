#ifndef CHRONOTRACE_MERKLE_H
#define CHRONOTRACE_MERKLE_H

#include "../../include/common.h"

typedef struct {
    char filename[CT_MAX_NAME_LEN];
    char hash_hex[CT_SHA256_HEX_LEN];
} ct_merkle_leaf_t;

typedef struct ct_merkle_node ct_merkle_node_t;

struct ct_merkle_node {
    char hash_hex[CT_SHA256_HEX_LEN];
    ct_merkle_node_t *left;
    ct_merkle_node_t *right;
};

typedef struct {
    ct_merkle_leaf_t *leaves;
    size_t leaf_count;
    ct_merkle_node_t *root;
    char root_hash_hex[CT_SHA256_HEX_LEN];
} ct_merkle_tree_t;

ct_status_t ct_merkle_build(ct_merkle_tree_t *tree, const ct_merkle_leaf_t *leaves, size_t count);
bool ct_merkle_verify_leaf(const ct_merkle_tree_t *tree, size_t leaf_idx, const char *file_hash_hex);
int32_t ct_merkle_find_changed_file(const ct_merkle_tree_t *tree, const char **current_file_hashes, size_t count);
void ct_merkle_free(ct_merkle_tree_t *tree);

#endif /* CHRONOTRACE_MERKLE_H */
