/*
 * EmberLite crypto - CRC-8 / CRC-16-CCITT / CRC-32 (IEEE)
 */
#pragma once

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/* --- CRC-8 (polynomial 0x07, init 0x00, MSB-first, no reflection) --- */
typedef struct {
    uint8_t crc;
} ember_crypto_crc8_ctx;

void ember_crypto_crc8_init(ember_crypto_crc8_ctx* ctx);
void ember_crypto_crc8_update(ember_crypto_crc8_ctx* ctx, const void* data, size_t len);
uint8_t ember_crypto_crc8_final(const ember_crypto_crc8_ctx* ctx);
uint8_t ember_crypto_crc8_compute(const void* data, size_t len);

/* --- CRC-16-CCITT (poly 0x1021, init 0xFFFF) --- */
typedef struct {
    uint16_t crc;
} ember_crypto_crc16_ctx;

void ember_crypto_crc16_init(ember_crypto_crc16_ctx* ctx);
void ember_crypto_crc16_update(ember_crypto_crc16_ctx* ctx, const void* data, size_t len);
uint16_t ember_crypto_crc16_final(const ember_crypto_crc16_ctx* ctx);
uint16_t ember_crypto_crc16_compute(const void* data, size_t len);

/* --- CRC-32 (Ethernet / IEEE 802.3, poly 0x04C11DB7 reflected) --- */
typedef struct {
    uint32_t crc;
} ember_crypto_crc32_ctx;

void ember_crypto_crc32_init(ember_crypto_crc32_ctx* ctx);
void ember_crypto_crc32_update(ember_crypto_crc32_ctx* ctx, const void* data, size_t len);
uint32_t ember_crypto_crc32_final(const ember_crypto_crc32_ctx* ctx);
uint32_t ember_crypto_crc32_compute(const void* data, size_t len);

#ifdef __cplusplus
}
#endif
