/*
 * EmberLite crypto - MD5 (RFC 1321)
 */
#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define EMBER_CRYPTO_MD5_DIGEST_SIZE 16

typedef struct {
    uint32_t state[4];
    uint32_t count[2];
    uint8_t buffer[64];
} ember_crypto_md5_ctx;

void ember_crypto_md5_init(ember_crypto_md5_ctx* ctx);
void ember_crypto_md5_update(ember_crypto_md5_ctx* ctx, const void* data, size_t len);
void ember_crypto_md5_final(ember_crypto_md5_ctx* ctx, uint8_t out_digest[EMBER_CRYPTO_MD5_DIGEST_SIZE]);
void ember_crypto_md5_compute(const void* data, size_t len, uint8_t out_digest[EMBER_CRYPTO_MD5_DIGEST_SIZE]);

#ifdef __cplusplus
}
#endif
