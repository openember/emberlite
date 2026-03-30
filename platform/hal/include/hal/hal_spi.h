/*
 * EmberLite HAL - SPI (Linux spidev)
 */
#pragma once

#include "hal/hal.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct hal_spi_handle_impl* hal_spi_handle_t;

typedef struct {
    const char* path;      /* e.g. "/dev/spidev0.0" */
    uint32_t speed_hz;     /* e.g. 1000000 */
    uint8_t mode;          /* 0..3 */
    uint8_t bits_per_word; /* usually 8 */
} hal_spi_config_t;

hal_status_t hal_spi_open(const hal_spi_config_t* config, hal_spi_handle_t* out_handle);
hal_status_t hal_spi_close(hal_spi_handle_t handle);

hal_status_t hal_spi_transfer(hal_spi_handle_t handle,
                              const uint8_t* tx,
                              uint8_t* rx,
                              size_t len);

int hal_spi_get_fd(hal_spi_handle_t handle);

#ifdef __cplusplus
}
#endif

