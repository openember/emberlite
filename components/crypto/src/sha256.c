/*
 * SHA-256 (FIPS 180-4)
 */

#include "ember/crypto_sha256.h"

#include <string.h>

#define SHA256_ROTR(x, n) (((x) >> (n)) | ((x) << (32 - (n))))

static const uint32_t sha256_k[64] = {
    0x428a2f98u, 0x71374491u, 0xb5c0fbcfu, 0xe9b5dba5u, 0x3956c25bu, 0x59f111f1u, 0x923f82a4u, 0xab1c5ed5u,
    0xd807aa98u, 0x12835b01u, 0x243185beu, 0x550c7dc3u, 0x72be5d74u, 0x80deb1feu, 0x9bdc06a7u, 0xc19bf174u,
    0xe49b69c1u, 0xefbe4786u, 0x0fc19dc6u, 0x240ca1ccu, 0x2de92c6fu, 0x4a7484aau, 0x5cb0a9dcu, 0x76f988dau,
    0x983e5152u, 0xa831c66du, 0xb00327c8u, 0xbf597fc7u, 0xc6e00bf3u, 0xd5a79147u, 0x06ca6351u, 0x14292967u,
    0x27b70a85u, 0x2e1b2138u, 0x4d2c6dfcu, 0x53380d13u, 0x650a7354u, 0x766a0abbu, 0x81c2c92eu, 0x92722c85u,
    0xa2bfe8a1u, 0xa81a664bu, 0xc24b8b70u, 0xc76c51a3u, 0xd192e819u, 0xd6990624u, 0xf40e3585u, 0x106aa070u,
    0x19a4c116u, 0x1e376c08u, 0x2748774cu, 0x34b0bcb5u, 0x391c0cb3u, 0x4ed8aa4au, 0x5b9cca4fu, 0x682e6ff3u,
    0x748f82eeu, 0x78a5636fu, 0x84c87814u, 0x8cc70208u, 0x90befffau, 0xa4506cebu, 0xbef9a3f7u, 0xc67178f2u
};

#define SHA256_CH(x, y, z) (((x) & (y)) ^ ((~(x)) & (z)))
#define SHA256_MAJ(x, y, z) (((x) & (y)) ^ ((x) & (z)) ^ ((y) & (z)))
#define SHA256_EP0(x) (SHA256_ROTR((x), 2) ^ SHA256_ROTR((x), 13) ^ SHA256_ROTR((x), 22))
#define SHA256_EP1(x) (SHA256_ROTR((x), 6) ^ SHA256_ROTR((x), 11) ^ SHA256_ROTR((x), 25))
#define SHA256_SIG0(x) (SHA256_ROTR((x), 7) ^ SHA256_ROTR((x), 18) ^ ((x) >> 3))
#define SHA256_SIG1(x) (SHA256_ROTR((x), 17) ^ SHA256_ROTR((x), 19) ^ ((x) >> 10))

static void sha256_transform(uint32_t state[8], const uint8_t block[64])
{
    uint32_t w[64];
    uint32_t a = state[0];
    uint32_t b = state[1];
    uint32_t c = state[2];
    uint32_t d = state[3];
    uint32_t e = state[4];
    uint32_t f = state[5];
    uint32_t g = state[6];
    uint32_t h = state[7];

    for (int i = 0; i < 16; i++) {
        w[i] = ((uint32_t)block[i * 4 + 0] << 24) | ((uint32_t)block[i * 4 + 1] << 16) | ((uint32_t)block[i * 4 + 2] << 8)
            | (uint32_t)block[i * 4 + 3];
    }
    for (int i = 16; i < 64; i++) {
        w[i] = SHA256_SIG1(w[i - 2]) + w[i - 7] + SHA256_SIG0(w[i - 15]) + w[i - 16];
    }

    for (int i = 0; i < 64; i++) {
        uint32_t t1 = h + SHA256_EP1(e) + SHA256_CH(e, f, g) + sha256_k[i] + w[i];
        uint32_t t2 = SHA256_EP0(a) + SHA256_MAJ(a, b, c);
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

void ember_crypto_sha256_init(ember_crypto_sha256_ctx* ctx)
{
    ctx->state[0] = 0x6a09e667u;
    ctx->state[1] = 0xbb67ae85u;
    ctx->state[2] = 0x3c6ef372u;
    ctx->state[3] = 0xa54ff53au;
    ctx->state[4] = 0x510e527fu;
    ctx->state[5] = 0x9b05688cu;
    ctx->state[6] = 0x1f83d9abu;
    ctx->state[7] = 0x5be0cd19u;
    ctx->bit_count = 0;
    ctx->buffer_len = 0;
    memset(ctx->buffer, 0, sizeof(ctx->buffer));
}

void ember_crypto_sha256_update(ember_crypto_sha256_ctx* ctx, const void* data, size_t len)
{
    const uint8_t* p = (const uint8_t*)data;
    ctx->bit_count += (uint64_t)len * 8u;

    while (len > 0) {
        size_t space = 64u - ctx->buffer_len;
        size_t copy = len < space ? len : space;
        memcpy(ctx->buffer + ctx->buffer_len, p, copy);
        ctx->buffer_len += copy;
        p += copy;
        len -= copy;

        if (ctx->buffer_len == 64) {
            sha256_transform(ctx->state, ctx->buffer);
            ctx->buffer_len = 0;
        }
    }
}

void ember_crypto_sha256_final(ember_crypto_sha256_ctx* ctx, uint8_t out_digest[EMBER_CRYPTO_SHA256_DIGEST_SIZE])
{
    uint64_t total_bits = ctx->bit_count;
    uint8_t block[64];
    size_t idx = ctx->buffer_len;

    memcpy(block, ctx->buffer, idx);
    block[idx++] = 0x80u;
    if (idx <= 56) {
        memset(block + idx, 0, 56 - idx);
    } else {
        memset(block + idx, 0, 64 - idx);
        sha256_transform(ctx->state, block);
        memset(block, 0, 56);
    }

    block[56] = (uint8_t)((total_bits >> 56) & 0xFFu);
    block[57] = (uint8_t)((total_bits >> 48) & 0xFFu);
    block[58] = (uint8_t)((total_bits >> 40) & 0xFFu);
    block[59] = (uint8_t)((total_bits >> 32) & 0xFFu);
    block[60] = (uint8_t)((total_bits >> 24) & 0xFFu);
    block[61] = (uint8_t)((total_bits >> 16) & 0xFFu);
    block[62] = (uint8_t)((total_bits >> 8) & 0xFFu);
    block[63] = (uint8_t)(total_bits & 0xFFu);

    sha256_transform(ctx->state, block);

    for (int i = 0; i < 8; i++) {
        out_digest[i * 4 + 0] = (uint8_t)((ctx->state[i] >> 24) & 0xFFu);
        out_digest[i * 4 + 1] = (uint8_t)((ctx->state[i] >> 16) & 0xFFu);
        out_digest[i * 4 + 2] = (uint8_t)((ctx->state[i] >> 8) & 0xFFu);
        out_digest[i * 4 + 3] = (uint8_t)(ctx->state[i] & 0xFFu);
    }
}

void ember_crypto_sha256_compute(const void* data, size_t len, uint8_t out_digest[EMBER_CRYPTO_SHA256_DIGEST_SIZE])
{
    ember_crypto_sha256_ctx ctx;
    ember_crypto_sha256_init(&ctx);
    ember_crypto_sha256_update(&ctx, data, len);
    ember_crypto_sha256_final(&ctx, out_digest);
}
