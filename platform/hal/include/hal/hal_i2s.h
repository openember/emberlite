/*
 * EmberLite HAL - I2S / PCM (ALSA when available)
 */
#pragma once

#include "hal/hal.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct hal_i2s_handle_impl* hal_i2s_handle_t;

typedef struct {
    const char* device_name;   /* "default" or "hw:0,0" */
    uint32_t sample_rate;      /* 44100, 48000 */
    uint8_t channels;          /* 1 or 2 */
    uint8_t bits_per_sample;   /* 16, 24, 32 */
    uint8_t capture;           /* 0 = playback (write), 1 = capture (read) */
} hal_i2s_config_t;

hal_status_t hal_i2s_open(const hal_i2s_config_t* cfg, hal_i2s_handle_t* out_handle);
hal_status_t hal_i2s_close(hal_i2s_handle_t handle);

/* frames: number of PCM frames; buffer size must be >= frames * frame_bytes */
hal_status_t hal_i2s_write(hal_i2s_handle_t handle,
                           const uint8_t* buffer,
                           size_t frames,
                           int32_t timeout_ms);
hal_status_t hal_i2s_read(hal_i2s_handle_t handle,
                          uint8_t* buffer,
                          size_t frames,
                          int32_t timeout_ms);

hal_status_t hal_i2s_start(hal_i2s_handle_t handle);
hal_status_t hal_i2s_stop(hal_i2s_handle_t handle);

int hal_i2s_get_fd(hal_i2s_handle_t handle);

#ifdef __cplusplus
}
#endif
