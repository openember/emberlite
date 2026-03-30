/*
 * CRC-8 / CRC-16-CCITT / CRC-32
 */

#include "ember/crypto_crc.h"

/* --- CRC-8: polynomial 0x07 --- */
static uint8_t crc8_table[256];

static void crc8_build_table(void)
{
    const uint8_t poly = 0x07u;
    for (int i = 0; i < 256; i++) {
        uint8_t c = (uint8_t)i;
        for (int j = 0; j < 8; j++) {
            if (c & 0x80u) {
                c = (uint8_t)((c << 1) ^ poly);
            } else {
                c <<= 1;
            }
        }
        crc8_table[i] = c;
    }
}

static int crc8_table_ready;

static void crc8_ensure_table(void)
{
    if (!crc8_table_ready) {
        crc8_build_table();
        crc8_table_ready = 1;
    }
}

void ember_crypto_crc8_init(ember_crypto_crc8_ctx* ctx)
{
    crc8_ensure_table();
    ctx->crc = 0x00u;
}

void ember_crypto_crc8_update(ember_crypto_crc8_ctx* ctx, const void* data, size_t len)
{
    const uint8_t* p = (const uint8_t*)data;
    uint8_t c = ctx->crc;
    for (size_t i = 0; i < len; i++) {
        c = crc8_table[c ^ p[i]];
    }
    ctx->crc = c;
}

uint8_t ember_crypto_crc8_final(const ember_crypto_crc8_ctx* ctx)
{
    return ctx->crc;
}

uint8_t ember_crypto_crc8_compute(const void* data, size_t len)
{
    ember_crypto_crc8_ctx ctx;
    ember_crypto_crc8_init(&ctx);
    ember_crypto_crc8_update(&ctx, data, len);
    return ember_crypto_crc8_final(&ctx);
}

/* --- CRC-16-CCITT: poly 0x1021, init 0xFFFF --- */
static uint16_t crc16_table[256];

static void crc16_build_table(void)
{
    const uint16_t poly = 0x1021u;
    for (int i = 0; i < 256; i++) {
        uint16_t c = (uint16_t)((uint16_t)i << 8);
        for (int j = 0; j < 8; j++) {
            if (c & 0x8000u) {
                c = (uint16_t)((c << 1) ^ poly);
            } else {
                c <<= 1;
            }
        }
        crc16_table[i] = c;
    }
}

static int crc16_table_ready;

static void crc16_ensure_table(void)
{
    if (!crc16_table_ready) {
        crc16_build_table();
        crc16_table_ready = 1;
    }
}

void ember_crypto_crc16_init(ember_crypto_crc16_ctx* ctx)
{
    crc16_ensure_table();
    ctx->crc = 0xFFFFu;
}

void ember_crypto_crc16_update(ember_crypto_crc16_ctx* ctx, const void* data, size_t len)
{
    const uint8_t* p = (const uint8_t*)data;
    uint16_t c = ctx->crc;
    for (size_t i = 0; i < len; i++) {
        c = (uint16_t)((c << 8) ^ crc16_table[((c >> 8) ^ p[i]) & 0xFFu]);
    }
    ctx->crc = c;
}

uint16_t ember_crypto_crc16_final(const ember_crypto_crc16_ctx* ctx)
{
    return ctx->crc;
}

uint16_t ember_crypto_crc16_compute(const void* data, size_t len)
{
    ember_crypto_crc16_ctx ctx;
    ember_crypto_crc16_init(&ctx);
    ember_crypto_crc16_update(&ctx, data, len);
    return ember_crypto_crc16_final(&ctx);
}

/* --- CRC-32 (Ethernet): reflected poly 0xEDB88320, init 0xFFFFFFFF, xorout 0xFFFFFFFF --- */
static uint32_t crc32_table[256];

static void crc32_build_table(void)
{
    const uint32_t poly = 0xEDB88320u;
    for (uint32_t i = 0; i < 256; i++) {
        uint32_t c = i;
        for (int j = 0; j < 8; j++) {
            if (c & 1u) {
                c = (c >> 1) ^ poly;
            } else {
                c >>= 1;
            }
        }
        crc32_table[i] = c;
    }
}

static int crc32_table_ready;

static void crc32_ensure_table(void)
{
    if (!crc32_table_ready) {
        crc32_build_table();
        crc32_table_ready = 1;
    }
}

void ember_crypto_crc32_init(ember_crypto_crc32_ctx* ctx)
{
    crc32_ensure_table();
    ctx->crc = 0xFFFFFFFFu;
}

void ember_crypto_crc32_update(ember_crypto_crc32_ctx* ctx, const void* data, size_t len)
{
    const uint8_t* p = (const uint8_t*)data;
    uint32_t c = ctx->crc;
    for (size_t i = 0; i < len; i++) {
        uint8_t idx = (uint8_t)((c ^ p[i]) & 0xFFu);
        c = (c >> 8) ^ crc32_table[idx];
    }
    ctx->crc = c;
}

uint32_t ember_crypto_crc32_final(const ember_crypto_crc32_ctx* ctx)
{
    return ctx->crc ^ 0xFFFFFFFFu;
}

uint32_t ember_crypto_crc32_compute(const void* data, size_t len)
{
    ember_crypto_crc32_ctx ctx;
    ember_crypto_crc32_init(&ctx);
    ember_crypto_crc32_update(&ctx, data, len);
    return ember_crypto_crc32_final(&ctx);
}
