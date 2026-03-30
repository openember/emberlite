/*
 * EmberLite HAL - CAN (Linux SocketCAN)
 */
#pragma once

#include "hal/hal.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct hal_can_handle_impl* hal_can_handle_t;

typedef struct {
    const char* ifname;     /* e.g. "can0" */
    uint32_t bitrate;       /* informational only (interface must be configured externally) */
    uint32_t filter_id;     /* optional */
} hal_can_config_t;

typedef struct {
    uint32_t id;
    uint8_t dlc;
    uint8_t data[8];
    uint8_t is_rtr;
} hal_can_frame_t;

hal_status_t hal_can_open(const hal_can_config_t* config, hal_can_handle_t* out_handle);
hal_status_t hal_can_close(hal_can_handle_t handle);

hal_status_t hal_can_send(hal_can_handle_t handle, const hal_can_frame_t* frame);
hal_status_t hal_can_receive(hal_can_handle_t handle, hal_can_frame_t* frame, int32_t timeout_ms);

int hal_can_get_fd(hal_can_handle_t handle);

#ifdef __cplusplus
}
#endif

