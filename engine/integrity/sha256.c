#include "sha256.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define CH(x,y,z) (((x) & (y)) ^ (~(x) & (z)))
#define MAJ(x,y,z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define ROTR(n,x) (((x) >> (n)) | ((x) << (32 - (n))))
#define SIGMA0(x) (ROTR(2, (x)) ^ ROTR(13, (x)) ^ ROTR(22, (x)))
#define SIGMA1(x) (ROTR(6, (x)) ^ ROTR(11, (x)) ^ ROTR(25, (x)))
#define SIGMA_L0(x) (ROTR(7, (x)) ^ ROTR(18, (x)) ^ ((x) >> 3))
#define SIGMA_L1(x) (ROTR(17, (x)) ^ ROTR(19, (x)) ^ ((x) >> 10))

static const uint32_t K[64] = {
    0x428a2f98, 0x71374491, 0xb5c0fbcf, 0xe9b5dba5, 0x3956c25b, 0x59f111f1, 0x923f82a4, 0xab1c5ed5,
    0xd807aa98, 0x12835b01, 0x243185be, 0x550c7dc3, 0x72be5d74, 0x80deb1fe, 0x9bdc06a7, 0xc19bf174,
    0xe49b69c1, 0xefbe4786, 0x0fc19dc6, 0x240ca1cc, 0x2de92c6f, 0x4a7484aa, 0x5cb0a9dc, 0x76f988da,
    0x983e5152, 0xa831c66d, 0xb00327c8, 0xbf597fc7, 0xc6e00bf3, 0xd5a79147, 0x06ca6351, 0x14292967,
    0x27b70a85, 0x2e1b2138, 0x4d2c6dfc, 0x53380d13, 0x650a7354, 0x766a0abb, 0x81c2c92e, 0x92722c85,
    0xa2bfe8a1, 0xa81a664b, 0xc24b8b70, 0xc76c51a3, 0xd192e819, 0xd6990624, 0xf40e3585, 0x106aa070,
    0x19a4c116, 0x1e376c08, 0x2748774c, 0x34b0bcb5, 0x391c0cb3, 0x4ed8aa4a, 0x5b9cca4f, 0x682e6ff3,
    0x748f82ee, 0x78a5636f, 0x84c87814, 0x8cc70208, 0x90befffa, 0xa4506ceb, 0xbef9a3f7, 0xc67178f2
};

void ct_sha256_init(ct_sha256_ctx_t *ctx) {
    if (!ctx) return;
    ctx->state[0] = 0x6a09e667;
    ctx->state[1] = 0xbb67ae85;
    ctx->state[2] = 0x3c6ef372;
    ctx->state[3] = 0xa54ff53a;
    ctx->state[4] = 0x510e527f;
    ctx->state[5] = 0x9b05688c;
    ctx->state[6] = 0x1f83d9ab;
    ctx->state[7] = 0x5be0cd19;
    ctx->count = 0;
}

static void ct_sha256_transform(uint32_t state[8], const uint8_t data[64]) {
    uint32_t a = state[0], b = state[1], c = state[2], d = state[3];
    uint32_t e = state[4], f = state[5], g = state[6], h = state[7];
    uint32_t w[64];

    for (int i = 0; i < 16; i++) {
        w[i] = ((uint32_t)data[i * 4] << 24) |
               ((uint32_t)data[i * 4 + 1] << 16) |
               ((uint32_t)data[i * 4 + 2] << 8) |
               ((uint32_t)data[i * 4 + 3]);
    }

    for (int i = 16; i < 64; i++) {
        w[i] = SIGMA_L1(w[i - 2]) + w[i - 7] + SIGMA_L0(w[i - 15]) + w[i - 16];
    }

    for (int i = 0; i < 64; i++) {
        uint32_t t1 = h + SIGMA1(e) + CH(e, f, g) + K[i] + w[i];
        uint32_t t2 = SIGMA0(a) + MAJ(a, b, c);
        h = g;
        g = f;
        f = e;
        e = d + t1;
        d = c;
        c = b;
        b = a;
        a = t1 + t2;
    }

    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
    state[4] += e;
    state[5] += f;
    state[6] += g;
    state[7] += h;
}

void ct_sha256_update(ct_sha256_ctx_t *ctx, const uint8_t *data, size_t len) {
    if (!ctx || !data || len == 0) return;

    size_t buffer_idx = (size_t)((ctx->count >> 3) & 0x3F);
    ctx->count += (uint64_t)len << 3;

    size_t part_len = 64 - buffer_idx;
    size_t i = 0;

    if (len >= part_len) {
        memcpy(&ctx->buffer[buffer_idx], data, part_len);
        ct_sha256_transform(ctx->state, ctx->buffer);
        for (i = part_len; i + 63 < len; i += 64) {
            ct_sha256_transform(ctx->state, &data[i]);
        }
        buffer_idx = 0;
    }

    memcpy(&ctx->buffer[buffer_idx], &data[i], len - i);
}

void ct_sha256_final(ct_sha256_ctx_t *ctx, uint8_t hash[32]) {
    if (!ctx || !hash) return;

    uint8_t final_count[8];
    for (int i = 0; i < 8; i++) {
        final_count[i] = (uint8_t)((ctx->count >> ((7 - i) * 8)) & 0xFF);
    }

    size_t buffer_idx = (size_t)((ctx->count >> 3) & 0x3F);
    size_t pad_len = (buffer_idx < 56) ? (56 - buffer_idx) : (120 - buffer_idx);

    static const uint8_t padding[64] = {
        0x80, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0
    };

    ct_sha256_update(ctx, padding, pad_len);
    ct_sha256_update(ctx, final_count, 8);

    for (int i = 0; i < 8; i++) {
        hash[i * 4]     = (uint8_t)((ctx->state[i] >> 24) & 0xFF);
        hash[i * 4 + 1] = (uint8_t)((ctx->state[i] >> 16) & 0xFF);
        hash[i * 4 + 2] = (uint8_t)((ctx->state[i] >> 8) & 0xFF);
        hash[i * 4 + 3] = (uint8_t)(ctx->state[i] & 0xFF);
    }
}

void ct_sha256_buffer_hex(const uint8_t *data, size_t len, char hex_out[CT_SHA256_HEX_LEN]) {
    if (!hex_out) return;
    ct_sha256_ctx_t ctx;
    uint8_t hash[32];
    ct_sha256_init(&ctx);
    ct_sha256_update(&ctx, data, len);
    ct_sha256_final(&ctx, hash);

    for (int i = 0; i < 32; i++) {
        sprintf(&hex_out[i * 2], "%02x", hash[i]);
    }
    hex_out[64] = '\0';
}

ct_status_t ct_sha256_file_hex(const char *filepath, char hex_out[CT_SHA256_HEX_LEN]) {
    if (!filepath || !hex_out) return CT_ERR_INVARG;

    FILE *f = fopen(filepath, "rb");
    if (!f) return CT_ERR_IO;

    ct_sha256_ctx_t ctx;
    ct_sha256_init(&ctx);

    uint8_t buffer[8192];
    size_t bytes_read = 0;
    while ((bytes_read = fread(buffer, 1, sizeof(buffer), f)) > 0) {
        ct_sha256_update(&ctx, buffer, bytes_read);
    }

    fclose(f);

    uint8_t hash[32];
    ct_sha256_final(&ctx, hash);

    for (int i = 0; i < 32; i++) {
        sprintf(&hex_out[i * 2], "%02x", hash[i]);
    }
    hex_out[64] = '\0';
    return CT_OK;
}
