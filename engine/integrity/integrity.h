#ifndef CHRONOTRACE_INTEGRITY_H
#define CHRONOTRACE_INTEGRITY_H

#include "merkle.h"
#include "sha256.h"

typedef struct {
    char filename[CT_MAX_NAME_LEN];
    char baseline_hash[CT_SHA256_HEX_LEN];
    char current_hash[CT_SHA256_HEX_LEN];
    bool is_match;
} ct_file_verification_t;

typedef struct {
    ct_merkle_tree_t baseline_tree;
    char baseline_manifest_hash[CT_SHA256_HEX_LEN];
    size_t file_count;
} ct_integrity_manifest_t;

typedef struct {
    ct_file_verification_t *files;
    size_t file_count;
    char baseline_root[CT_SHA256_HEX_LEN];
    char current_root[CT_SHA256_HEX_LEN];
    bool all_files_match;
    bool manifest_root_matches;
} ct_integrity_report_t;

ct_status_t ct_integrity_init_manifest(ct_integrity_manifest_t *manifest,
                                       const char **filenames,
                                       const char **file_paths,
                                       size_t count);

ct_status_t ct_integrity_verify(const ct_integrity_manifest_t *manifest,
                                const char **file_paths,
                                size_t count,
                                ct_integrity_report_t *out_report);

void ct_integrity_report_free(ct_integrity_report_t *report);
void ct_integrity_manifest_free(ct_integrity_manifest_t *manifest);

#endif /* CHRONOTRACE_INTEGRITY_H */
