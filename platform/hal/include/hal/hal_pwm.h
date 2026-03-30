/*
 * EmberLite HAL - PWM (Linux sysfs pwmchip)
 */
#pragma once

#include "hal/hal.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct hal_pwm_handle_impl* hal_pwm_handle_t;

typedef struct {
    const char* chip_path; /* e.g. "/sys/class/pwm/pwmchip0" */
    uint32_t channel;      /* PWM channel to export (usually 0) */
} hal_pwm_config_t;

hal_status_t hal_pwm_open(const hal_pwm_config_t* config, hal_pwm_handle_t* out_handle);
hal_status_t hal_pwm_close(hal_pwm_handle_t handle);

hal_status_t hal_pwm_set_frequency(hal_pwm_handle_t handle, uint32_t hz);
hal_status_t hal_pwm_set_duty_cycle(hal_pwm_handle_t handle, float percent);
hal_status_t hal_pwm_enable(hal_pwm_handle_t handle);
hal_status_t hal_pwm_disable(hal_pwm_handle_t handle);

int hal_pwm_get_fd(hal_pwm_handle_t handle);

#ifdef __cplusplus
}
#endif
