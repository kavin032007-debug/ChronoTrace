#ifndef CHRONOTRACE_SHA256_H
#define CHRONOTRACE_SHA256_H

#include "../../include/common.h"

typedef struct {
    uint32_t state[8];
    uint64_t count;
    uint8_t buffer[64];
} ct_sha256_ctx_t;

void ct_sha256_init(ct_sha256_ctx_t *ctx);
void ct_sha256_update(ct_sha256_ctx_t *ctx, const uint8_t *data, size_t len);
void ct_sha256_final(ct_sha256_ctx_t *ctx, uint8_t hash[32]);

/* Helper for hashing memory buffer to 64-char hex string */
void ct_sha256_buffer_hex(const uint8_t *data, size_t len, char hex_out[CT_SHA256_HEX_LEN]);

/* Helper for hashing file content to 64-char hex string */
ct_status_t ct_sha256_file_hex(const char *filepath, char hex_out[CT_SHA256_HEX_LEN]);

#endif /* CHRONOTRACE_SHA256_H */
