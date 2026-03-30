/*
 * EmberLite HAL - Input (Linux evdev)
 */
#pragma once

#include "hal/hal.h"

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct hal_input_handle_impl* hal_input_handle_t;

typedef struct {
    const char* device_path; /* e.g. "/dev/input/event0" */
} hal_input_config_t;

typedef struct {
    uint64_t time_sec;
    uint64_t time_usec;
    uint16_t type;
    uint16_t code;
    int32_t value;
} hal_input_event_t;

hal_status_t hal_input_open(const hal_input_config_t* config, hal_input_handle_t* out_handle);
hal_status_t hal_input_close(hal_input_handle_t handle);

/* timeout_ms: 0 = non-blocking, -1 = block until one event */
hal_status_t hal_input_read_event(hal_input_handle_t handle,
                                  hal_input_event_t* out_event,
                                  int32_t timeout_ms);

int hal_input_get_fd(hal_input_handle_t handle);

#ifdef __cplusplus
}
#endif
