#include "merkle.h"
#include "sha256.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static ct_merkle_node_t* ct_merkle_create_node(const char *hash_hex, ct_merkle_node_t *left, ct_merkle_node_t *right) {
    ct_merkle_node_t *node = (ct_merkle_node_t*)malloc(sizeof(ct_merkle_node_t));
    if (!node) return NULL;
    strncpy(node->hash_hex, hash_hex, CT_SHA256_HEX_LEN - 1);
    node->hash_hex[CT_SHA256_HEX_LEN - 1] = '\0';
    node->left = left;
    node->right = right;
    return node;
}

static void ct_merkle_combine_hashes(const char *left_hex, const char *right_hex, char out_hex[CT_SHA256_HEX_LEN]) {
    char combined[CT_SHA256_HEX_LEN * 2];
    snprintf(combined, sizeof(combined), "%s%s", left_hex, right_hex);
    ct_sha256_buffer_hex((const uint8_t*)combined, strlen(combined), out_hex);
}

ct_status_t ct_merkle_build(ct_merkle_tree_t *tree, const ct_merkle_leaf_t *leaves, size_t count) {
    if (!tree) return CT_ERR_INVARG;
    memset(tree, 0, sizeof(ct_merkle_tree_t));

    if (count == 0) {
        /* Empty tree root hash */
        ct_sha256_buffer_hex((const uint8_t*)"", 0, tree->root_hash_hex);
        return CT_OK;
    }

    tree->leaves = (ct_merkle_leaf_t*)malloc(count * sizeof(ct_merkle_leaf_t));
    if (!tree->leaves) return CT_ERR_NOMEM;
    memcpy(tree->leaves, leaves, count * sizeof(ct_merkle_leaf_t));
    tree->leaf_count = count;

    /* Build layer of leaf nodes */
    size_t layer_size = count;
    ct_merkle_node_t **layer = (ct_merkle_node_t**)malloc(layer_size * sizeof(ct_merkle_node_t*));
    if (!layer) {
        free(tree->leaves);
        return CT_ERR_NOMEM;
    }

    for (size_t i = 0; i < count; i++) {
        layer[i] = ct_merkle_create_node(leaves[i].hash_hex, NULL, NULL);
    }

    /* Build parent layers bottom-up */
    while (layer_size > 1) {
        size_t next_size = (layer_size + 1) / 2;
        ct_merkle_node_t **next_layer = (ct_merkle_node_t**)malloc(next_size * sizeof(ct_merkle_node_t*));
        if (!next_layer) {
            /* Cleanup on failure */
            free(layer);
            return CT_ERR_NOMEM;
        }

        for (size_t i = 0; i < next_size; i++) {
            size_t left_idx = i * 2;
            size_t right_idx = (i * 2 + 1 < layer_size) ? (i * 2 + 1) : left_idx;

            char parent_hash[CT_SHA256_HEX_LEN];
            ct_merkle_combine_hashes(layer[left_idx]->hash_hex, layer[right_idx]->hash_hex, parent_hash);

            next_layer[i] = ct_merkle_create_node(parent_hash, layer[left_idx], (left_idx != right_idx) ? layer[right_idx] : NULL);
        }

        free(layer);
        layer = next_layer;
        layer_size = next_size;
    }

    tree->root = layer[0];
    strncpy(tree->root_hash_hex, layer[0]->hash_hex, CT_SHA256_HEX_LEN - 1);
    tree->root_hash_hex[CT_SHA256_HEX_LEN - 1] = '\0';

    free(layer);
    return CT_OK;
}

bool ct_merkle_verify_leaf(const ct_merkle_tree_t *tree, size_t leaf_idx, const char *file_hash_hex) {
    if (!tree || leaf_idx >= tree->leaf_count || !file_hash_hex) return false;
    return strcmp(tree->leaves[leaf_idx].hash_hex, file_hash_hex) == 0;
}

int32_t ct_merkle_find_changed_file(const ct_merkle_tree_t *tree, const char **current_file_hashes, size_t count) {
    if (!tree || !current_file_hashes) return -1;
    size_t n = (count < tree->leaf_count) ? count : tree->leaf_count;

    for (size_t i = 0; i < n; i++) {
        if (strcmp(tree->leaves[i].hash_hex, current_file_hashes[i]) != 0) {
            return (int32_t)i;
        }
    }
    if (count != tree->leaf_count) return (int32_t)n;
    return -1;
}

static void ct_merkle_free_node(ct_merkle_node_t *node) {
    if (!node) return;
    ct_merkle_free_node(node->left);
    if (node->right != node->left) {
        ct_merkle_free_node(node->right);
    }
    free(node);
}

void ct_merkle_free(ct_merkle_tree_t *tree) {
    if (!tree) return;
    if (tree->leaves) {
        free(tree->leaves);
        tree->leaves = NULL;
    }
    if (tree->root) {
        ct_merkle_free_node(tree->root);
        tree->root = NULL;
    }
    tree->leaf_count = 0;
    tree->root_hash_hex[0] = '\0';
}
