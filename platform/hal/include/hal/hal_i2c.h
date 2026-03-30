/*
 * EmberLite HAL - I2C (Linux i2c-dev)
 */
#pragma once

#include "hal/hal.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct hal_i2c_handle_impl* hal_i2c_handle_t;

typedef struct {
    const char* path;       /* e.g. "/dev/i2c-1" */
    uint16_t slave_addr;    /* 7-bit address (0..0x7f) */
    uint32_t timeout_ms;    /* bus timeout hint (best-effort) */
} hal_i2c_config_t;

hal_status_t hal_i2c_open(const hal_i2c_config_t* config, hal_i2c_handle_t* out_handle);
hal_status_t hal_i2c_close(hal_i2c_handle_t handle);

hal_status_t hal_i2c_write(hal_i2c_handle_t handle, const uint8_t* data, size_t len);
hal_status_t hal_i2c_read(hal_i2c_handle_t handle, uint8_t* data, size_t len);

hal_status_t hal_i2c_write_reg(hal_i2c_handle_t handle, uint8_t reg, const uint8_t* data, size_t len);
hal_status_t hal_i2c_read_reg(hal_i2c_handle_t handle, uint8_t reg, uint8_t* data, size_t len);

int hal_i2c_get_fd(hal_i2c_handle_t handle);

#ifdef __cplusplus
}
#endif

