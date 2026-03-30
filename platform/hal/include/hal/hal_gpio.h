/*
 * EmberLite HAL - GPIO (Linux sysfs)
 *
 * Note: sysfs GPIO is deprecated on newer kernels, but kept here as a
 * zero-dependency userspace baseline. It uses open/read/write/close only.
 */
#pragma once

#include "hal/hal.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct hal_gpio_handle_impl* hal_gpio_handle_t;

typedef struct {
    uint32_t pin;          /* GPIO number */
    uint8_t direction;     /* 0=In, 1=Out */
    uint8_t pull_mode;     /* reserved, not supported via sysfs */
    uint8_t initial_level; /* only for output */
} hal_gpio_config_t;

hal_status_t hal_gpio_open(const hal_gpio_config_t* config, hal_gpio_handle_t* out_handle);
hal_status_t hal_gpio_close(hal_gpio_handle_t handle);

hal_status_t hal_gpio_read(hal_gpio_handle_t handle, uint8_t* out_level);
hal_status_t hal_gpio_write(hal_gpio_handle_t handle, uint8_t level);
hal_status_t hal_gpio_toggle(hal_gpio_handle_t handle);

int hal_gpio_get_fd(hal_gpio_handle_t handle);

#ifdef __cplusplus
}
#endif

