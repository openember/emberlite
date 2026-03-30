/*
 * MD5 message digest (RFC 1321)
 */

#include "ember/crypto_md5.h"

#include <string.h>

#define MD5_ROTL(x, n) (((x) << (n)) | ((x) >> (32 - (n))))

#define MD5_F(x, y, z) (((x) & (y)) | ((~(x)) & (z)))
#define MD5_G(x, y, z) (((x) & (z)) | ((y) & (~(z))))
#define MD5_H(x, y, z) ((x) ^ (y) ^ (z))
#define MD5_I(x, y, z) ((y) ^ ((x) | (~(z))))

#define MD5_STEP(f, a, b, c, d, x, t, s) \
    do { \
        (a) += f((b), (c), (d)) + (x) + (t); \
        (a) = MD5_ROTL((a), (s)); \
        (a) += (b); \
    } while (0)

static void md5_transform(uint32_t state[4], const uint8_t block[64])
{
    uint32_t a = state[0];
    uint32_t b = state[1];
    uint32_t c = state[2];
    uint32_t d = state[3];
    uint32_t x[16];

    for (int i = 0; i < 16; i++) {
        x[i] = (uint32_t)block[i * 4 + 0] | ((uint32_t)block[i * 4 + 1] << 8) | ((uint32_t)block[i * 4 + 2] << 16)
            | ((uint32_t)block[i * 4 + 3] << 24);
    }

    /* Round 1 */
    MD5_STEP(MD5_F, a, b, c, d, x[0], 0xd76aa478u, 7);
    MD5_STEP(MD5_F, d, a, b, c, x[1], 0xe8c7b756u, 12);
    MD5_STEP(MD5_F, c, d, a, b, x[2], 0x242070dbu, 17);
    MD5_STEP(MD5_F, b, c, d, a, x[3], 0xc1bdceeeu, 22);
    MD5_STEP(MD5_F, a, b, c, d, x[4], 0xf57c0fafu, 7);
    MD5_STEP(MD5_F, d, a, b, c, x[5], 0x4787c62au, 12);
    MD5_STEP(MD5_F, c, d, a, b, x[6], 0xa8304613u, 17);
    MD5_STEP(MD5_F, b, c, d, a, x[7], 0xfd469501u, 22);
    MD5_STEP(MD5_F, a, b, c, d, x[8], 0x698098d8u, 7);
    MD5_STEP(MD5_F, d, a, b, c, x[9], 0x8b44f7afu, 12);
    MD5_STEP(MD5_F, c, d, a, b, x[10], 0xffff5bb1u, 17);
    MD5_STEP(MD5_F, b, c, d, a, x[11], 0x895cd7beu, 22);
    MD5_STEP(MD5_F, a, b, c, d, x[12], 0x6b901122u, 7);
    MD5_STEP(MD5_F, d, a, b, c, x[13], 0xfd987193u, 12);
    MD5_STEP(MD5_F, c, d, a, b, x[14], 0xa679438eu, 17);
    MD5_STEP(MD5_F, b, c, d, a, x[15], 0x49b40821u, 22);

    /* Round 2 */
    MD5_STEP(MD5_G, a, b, c, d, x[1], 0xf61e2562u, 5);
    MD5_STEP(MD5_G, d, a, b, c, x[6], 0xc040b340u, 9);
    MD5_STEP(MD5_G, c, d, a, b, x[11], 0x265e5a51u, 14);
    MD5_STEP(MD5_G, b, c, d, a, x[0], 0xe9b6c7aau, 20);
    MD5_STEP(MD5_G, a, b, c, d, x[5], 0xd62f105du, 5);
    MD5_STEP(MD5_G, d, a, b, c, x[10], 0x02441453u, 9);
    MD5_STEP(MD5_G, c, d, a, b, x[15], 0xd8a1e681u, 14);
    MD5_STEP(MD5_G, b, c, d, a, x[4], 0xe7d3fbc8u, 20);
    MD5_STEP(MD5_G, a, b, c, d, x[9], 0x21e1cde6u, 5);
    MD5_STEP(MD5_G, d, a, b, c, x[14], 0xc33707d6u, 9);
    MD5_STEP(MD5_G, c, d, a, b, x[3], 0xf4d50d87u, 14);
    MD5_STEP(MD5_G, b, c, d, a, x[8], 0x455a14edu, 20);
    MD5_STEP(MD5_G, a, b, c, d, x[13], 0xa9e3e905u, 5);
    MD5_STEP(MD5_G, d, a, b, c, x[2], 0xfcefa3f8u, 9);
    MD5_STEP(MD5_G, c, d, a, b, x[7], 0x676f02d9u, 14);
    MD5_STEP(MD5_G, b, c, d, a, x[12], 0x8d2a4c8au, 20);

    /* Round 3 */
    MD5_STEP(MD5_H, a, b, c, d, x[5], 0xfffa3942u, 4);
    MD5_STEP(MD5_H, d, a, b, c, x[8], 0x8771f681u, 11);
    MD5_STEP(MD5_H, c, d, a, b, x[11], 0x6d9d6122u, 16);
    MD5_STEP(MD5_H, b, c, d, a, x[14], 0xfde5380cu, 23);
    MD5_STEP(MD5_H, a, b, c, d, x[1], 0xa4beea44u, 4);
    MD5_STEP(MD5_H, d, a, b, c, x[4], 0x4bdecfa9u, 11);
    MD5_STEP(MD5_H, c, d, a, b, x[7], 0xf6bb4b60u, 16);
    MD5_STEP(MD5_H, b, c, d, a, x[10], 0xbebfbc70u, 23);
    MD5_STEP(MD5_H, a, b, c, d, x[13], 0x289b7ec6u, 4);
    MD5_STEP(MD5_H, d, a, b, c, x[0], 0xeaa127fau, 11);
    MD5_STEP(MD5_H, c, d, a, b, x[3], 0xd4ef3085u, 16);
    MD5_STEP(MD5_H, b, c, d, a, x[6], 0x04881d05u, 23);
    MD5_STEP(MD5_H, a, b, c, d, x[9], 0xd9d4d039u, 4);
    MD5_STEP(MD5_H, d, a, b, c, x[12], 0xe6db99e5u, 11);
    MD5_STEP(MD5_H, c, d, a, b, x[15], 0x1fa27cf8u, 16);
    MD5_STEP(MD5_H, b, c, d, a, x[2], 0xc4ac5665u, 23);

    /* Round 4 */
    MD5_STEP(MD5_I, a, b, c, d, x[0], 0xf4292244u, 6);
    MD5_STEP(MD5_I, d, a, b, c, x[7], 0x432aff97u, 10);
    MD5_STEP(MD5_I, c, d, a, b, x[14], 0xab9423a7u, 15);
    MD5_STEP(MD5_I, b, c, d, a, x[5], 0xfc93a039u, 21);
    MD5_STEP(MD5_I, a, b, c, d, x[12], 0x655b59c3u, 6);
    MD5_STEP(MD5_I, d, a, b, c, x[3], 0x8f0ccc92u, 10);
    MD5_STEP(MD5_I, c, d, a, b, x[10], 0xffeff47du, 15);
    MD5_STEP(MD5_I, b, c, d, a, x[1], 0x85845dd1u, 21);
    MD5_STEP(MD5_I, a, b, c, d, x[8], 0x6fa87e4fu, 6);
    MD5_STEP(MD5_I, d, a, b, c, x[15], 0xfe2ce6e0u, 10);
    MD5_STEP(MD5_I, c, d, a, b, x[6], 0xa3014314u, 15);
    MD5_STEP(MD5_I, b, c, d, a, x[13], 0x4e0811a1u, 21);
    MD5_STEP(MD5_I, a, b, c, d, x[4], 0xf7537e82u, 6);
    MD5_STEP(MD5_I, d, a, b, c, x[11], 0xbd3af235u, 10);
    MD5_STEP(MD5_I, c, d, a, b, x[2], 0x2ad7d2bbu, 15);
    MD5_STEP(MD5_I, b, c, d, a, x[9], 0xeb86d391u, 21);

    state[0] += a;
    state[1] += b;
    state[2] += c;
    state[3] += d;
}

void ember_crypto_md5_init(ember_crypto_md5_ctx* ctx)
{
    ctx->state[0] = 0x67452301u;
    ctx->state[1] = 0xefcdab89u;
    ctx->state[2] = 0x98badcfeu;
    ctx->state[3] = 0x10325476u;
    ctx->count[0] = 0;
    ctx->count[1] = 0;
    memset(ctx->buffer, 0, sizeof(ctx->buffer));
}

void ember_crypto_md5_update(ember_crypto_md5_ctx* ctx, const void* data, size_t len)
{
    const uint8_t* p = (const uint8_t*)data;
    uint32_t idx = (uint32_t)((ctx->count[0] >> 3) & 0x3Fu);

    ctx->count[0] += (uint32_t)(len << 3);
    if (ctx->count[0] < (uint32_t)(len << 3)) {
        ctx->count[1]++;
    }
    ctx->count[1] += (uint32_t)(len >> 29);

    size_t i = 0;
    while (i < len) {
        size_t copy = (size_t)(64 - idx);
        if (copy > len - i) {
            copy = len - i;
        }
        memcpy(ctx->buffer + idx, p + i, copy);
        idx += (uint32_t)copy;
        i += copy;
        if (idx == 64) {
            md5_transform(ctx->state, ctx->buffer);
            idx = 0;
        }
    }
}

void ember_crypto_md5_final(ember_crypto_md5_ctx* ctx, uint8_t out_digest[EMBER_CRYPTO_MD5_DIGEST_SIZE])
{
    uint8_t len_bytes[8];
    uint32_t idx = (uint32_t)((ctx->count[0] >> 3) & 0x3Fu);
    uint32_t lo = ctx->count[0];
    uint32_t hi = ctx->count[1];
    for (int i = 0; i < 8; i++) {
        len_bytes[i] = (uint8_t)((i < 4) ? ((lo >> (8 * i)) & 0xFFu) : ((hi >> (8 * (i - 4))) & 0xFFu));
    }

    uint32_t padlen = (idx < 56) ? (56u - idx) : (120u - idx);
    uint8_t pad[64];
    pad[0] = 0x80u;
    if (padlen > 1u) {
        memset(pad + 1, 0, (size_t)padlen - 1u);
    }
    ember_crypto_md5_update(ctx, pad, padlen);
    ember_crypto_md5_update(ctx, len_bytes, 8);

    for (int i = 0; i < 4; i++) {
        out_digest[i * 4 + 0] = (uint8_t)(ctx->state[i] & 0xFFu);
        out_digest[i * 4 + 1] = (uint8_t)((ctx->state[i] >> 8) & 0xFFu);
        out_digest[i * 4 + 2] = (uint8_t)((ctx->state[i] >> 16) & 0xFFu);
        out_digest[i * 4 + 3] = (uint8_t)((ctx->state[i] >> 24) & 0xFFu);
    }
}

void ember_crypto_md5_compute(const void* data, size_t len, uint8_t out_digest[EMBER_CRYPTO_MD5_DIGEST_SIZE])
{
    ember_crypto_md5_ctx ctx;
    ember_crypto_md5_init(&ctx);
    ember_crypto_md5_update(&ctx, data, len);
    ember_crypto_md5_final(&ctx, out_digest);
}
