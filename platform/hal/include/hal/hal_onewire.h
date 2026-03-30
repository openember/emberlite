/*
 * EmberLite HAL - 1-Wire (Linux w1-gpio sysfs)
 */
#pragma once

#include "hal/hal.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct hal_onewire_handle_impl* hal_onewire_handle_t;

typedef struct {
    const char* bus_path;  /* e.g. "/sys/bus/w1/devices" */
    const char* device_id; /* e.g. "28-000001234567" */
} hal_onewire_config_t;

hal_status_t hal_onewire_open(const hal_onewire_config_t* cfg, hal_onewire_handle_t* out_handle);
hal_status_t hal_onewire_close(hal_onewire_handle_t handle);

hal_status_t hal_onewire_read_temp(hal_onewire_handle_t handle, float* temp_c);
hal_status_t hal_onewire_read_raw(hal_onewire_handle_t handle, uint8_t* buf, size_t len, size_t* out_len);

#ifdef __cplusplus
}
#endif
