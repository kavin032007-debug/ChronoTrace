#include "integrity.h"
#include <stdlib.h>
#include <string.h>

ct_status_t ct_integrity_init_manifest(ct_integrity_manifest_t *manifest,
                                       const char **filenames,
                                       const char **file_paths,
                                       size_t count) {
    if (!manifest || !filenames || !file_paths || count == 0) return CT_ERR_INVARG;
    memset(manifest, 0, sizeof(ct_integrity_manifest_t));

    ct_merkle_leaf_t *leaves = (ct_merkle_leaf_t*)malloc(count * sizeof(ct_merkle_leaf_t));
    if (!leaves) return CT_ERR_NOMEM;

    for (size_t i = 0; i < count; i++) {
        strncpy(leaves[i].filename, filenames[i], CT_MAX_NAME_LEN - 1);
        leaves[i].filename[CT_MAX_NAME_LEN - 1] = '\0';

        ct_status_t st = ct_sha256_file_hex(file_paths[i], leaves[i].hash_hex);
        if (st != CT_OK) {
            free(leaves);
            return st;
        }
    }

    ct_status_t st = ct_merkle_build(&manifest->baseline_tree, leaves, count);
    free(leaves);
    if (st != CT_OK) return st;

    strncpy(manifest->baseline_manifest_hash, manifest->baseline_tree.root_hash_hex, CT_SHA256_HEX_LEN - 1);
    manifest->baseline_manifest_hash[CT_SHA256_HEX_LEN - 1] = '\0';
    manifest->file_count = count;
    return CT_OK;
}

ct_status_t ct_integrity_verify(const ct_integrity_manifest_t *manifest,
                                const char **file_paths,
                                size_t count,
                                ct_integrity_report_t *out_report) {
    if (!manifest || !file_paths || !out_report) return CT_ERR_INVARG;
    memset(out_report, 0, sizeof(ct_integrity_report_t));

    if (count != manifest->file_count) {
        return CT_ERR_INVARG;
    }

    out_report->files = (ct_file_verification_t*)malloc(count * sizeof(ct_file_verification_t));
    if (!out_report->files) return CT_ERR_NOMEM;
    out_report->file_count = count;

    ct_merkle_leaf_t *cur_leaves = (ct_merkle_leaf_t*)malloc(count * sizeof(ct_merkle_leaf_t));
    if (!cur_leaves) {
        free(out_report->files);
        return CT_ERR_NOMEM;
    }

    bool all_match = true;
    for (size_t i = 0; i < count; i++) {
        ct_file_verification_t *fv = &out_report->files[i];
        strncpy(fv->filename, manifest->baseline_tree.leaves[i].filename, CT_MAX_NAME_LEN - 1);
        fv->filename[CT_MAX_NAME_LEN - 1] = '\0';

        strncpy(fv->baseline_hash, manifest->baseline_tree.leaves[i].hash_hex, CT_SHA256_HEX_LEN - 1);
        fv->baseline_hash[CT_SHA256_HEX_LEN - 1] = '\0';

        ct_status_t st = ct_sha256_file_hex(file_paths[i], fv->current_hash);
        if (st != CT_OK) {
            fv->current_hash[0] = '\0';
            fv->is_match = false;
            all_match = false;
        } else {
            fv->is_match = (strcmp(fv->baseline_hash, fv->current_hash) == 0);
            if (!fv->is_match) all_match = false;
        }

        strncpy(cur_leaves[i].filename, fv->filename, CT_MAX_NAME_LEN - 1);
        cur_leaves[i].filename[CT_MAX_NAME_LEN - 1] = '\0';
        strncpy(cur_leaves[i].hash_hex, fv->current_hash, CT_SHA256_HEX_LEN - 1);
        cur_leaves[i].hash_hex[CT_SHA256_HEX_LEN - 1] = '\0';
    }

    ct_merkle_tree_t cur_tree;
    ct_status_t st = ct_merkle_build(&cur_tree, cur_leaves, count);
    free(cur_leaves);
    if (st != CT_OK) {
        free(out_report->files);
        return st;
    }

    strncpy(out_report->baseline_root, manifest->baseline_manifest_hash, CT_SHA256_HEX_LEN - 1);
    out_report->baseline_root[CT_SHA256_HEX_LEN - 1] = '\0';
    strncpy(out_report->current_root, cur_tree.root_hash_hex, CT_SHA256_HEX_LEN - 1);
    out_report->current_root[CT_SHA256_HEX_LEN - 1] = '\0';

    out_report->all_files_match = all_match;
    out_report->manifest_root_matches = (strcmp(out_report->baseline_root, out_report->current_root) == 0);

    ct_merkle_free(&cur_tree);
    return CT_OK;
}

void ct_integrity_report_free(ct_integrity_report_t *report) {
    if (!report) return;
    if (report->files) {
        free(report->files);
        report->files = NULL;
    }
    report->file_count = 0;
}

void ct_integrity_manifest_free(ct_integrity_manifest_t *manifest) {
    if (!manifest) return;
    ct_merkle_free(&manifest->baseline_tree);
    manifest->baseline_manifest_hash[0] = '\0';
    manifest->file_count = 0;
}
