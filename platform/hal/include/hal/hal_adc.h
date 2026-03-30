/*
 * EmberLite HAL - ADC (Linux IIO sysfs)
 */
#pragma once

#include "hal/hal.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct hal_adc_handle_impl* hal_adc_handle_t;

typedef struct {
    const char* iio_path;   /* e.g. "/sys/bus/iio/devices/iio:device0" */
    uint32_t channel_index; /* 0 -> in_voltage0_raw / in_voltage0_scale */
} hal_adc_config_t;

hal_status_t hal_adc_open(const hal_adc_config_t* config, hal_adc_handle_t* out_handle);
hal_status_t hal_adc_close(hal_adc_handle_t handle);

hal_status_t hal_adc_read_raw(hal_adc_handle_t handle, uint32_t* out_value);
hal_status_t hal_adc_read_voltage(hal_adc_handle_t handle, uint32_t* out_mv);

#ifdef __cplusplus
}
#endif
