/*
 * EmberLite crypto - SHA-256 (FIPS 180-4)
 */
#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define EMBER_CRYPTO_SHA256_DIGEST_SIZE 32

typedef struct {
    uint32_t state[8];
    uint64_t bit_count;
    uint8_t buffer[64];
    size_t buffer_len;
} ember_crypto_sha256_ctx;

void ember_crypto_sha256_init(ember_crypto_sha256_ctx* ctx);
void ember_crypto_sha256_update(ember_crypto_sha256_ctx* ctx, const void* data, size_t len);
void ember_crypto_sha256_final(ember_crypto_sha256_ctx* ctx, uint8_t out_digest[EMBER_CRYPTO_SHA256_DIGEST_SIZE]);
void ember_crypto_sha256_compute(const void* data, size_t len, uint8_t out_digest[EMBER_CRYPTO_SHA256_DIGEST_SIZE]);

#ifdef __cplusplus
}
#endif
