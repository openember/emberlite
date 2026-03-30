/*
 * EmberLite HAL - System time, timers, watchdog
 */
#pragma once

#include "hal/hal.h"

#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

uint64_t hal_system_get_time_ms(void);
uint64_t hal_system_get_time_us(void);
uint64_t hal_system_get_uptime_ms(void);
/* Millidegree Celsius from thermal sysfs; 0 if unavailable */
uint64_t hal_system_get_cpu_temp(void);

void hal_system_sleep_ms(uint32_t ms);

typedef void* hal_timer_handle_t;

typedef struct {
    uint32_t interval_ms;
    uint32_t initial_ms;
    bool periodic;
} hal_timer_config_t;

typedef void (*hal_timer_callback_t)(hal_timer_handle_t handle, void* user_data);

hal_timer_handle_t hal_timer_create(const hal_timer_config_t* cfg,
                                    hal_timer_callback_t cb,
                                    void* user_data);
hal_status_t hal_timer_start(hal_timer_handle_t handle);
hal_status_t hal_timer_stop(hal_timer_handle_t handle);
void hal_timer_destroy(hal_timer_handle_t handle);
int hal_timer_get_fd(hal_timer_handle_t handle);

typedef struct hal_watchdog_handle_impl* hal_watchdog_handle_t;

hal_status_t hal_watchdog_open(const char* dev_path, hal_watchdog_handle_t* out_handle);
hal_status_t hal_watchdog_close(hal_watchdog_handle_t handle);
hal_status_t hal_watchdog_feed(hal_watchdog_handle_t handle);
hal_status_t hal_watchdog_set_timeout(hal_watchdog_handle_t handle, unsigned seconds);

#ifdef __cplusplus
}
#endif
