/*
 * EmberLite HAL - USB (libusb-1.0 when available)
 */
#pragma once

#include "hal/hal.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct hal_usb_handle_impl* hal_usb_handle_t;

typedef struct {
    uint16_t vendor_id;
    uint16_t product_id;
    int interface_number;
} hal_usb_config_t;

hal_status_t hal_usb_open(const hal_usb_config_t* config, hal_usb_handle_t* out_handle);
hal_status_t hal_usb_close(hal_usb_handle_t handle);

hal_status_t hal_usb_control_transfer(hal_usb_handle_t handle,
                                      uint8_t req_type,
                                      uint8_t req,
                                      uint16_t val,
                                      uint16_t idx,
                                      uint8_t* data,
                                      size_t len,
                                      int timeout_ms);

hal_status_t hal_usb_bulk_write(hal_usb_handle_t handle,
                                uint8_t endpoint,
                                const uint8_t* data,
                                size_t len,
                                int32_t timeout_ms);

hal_status_t hal_usb_bulk_read(hal_usb_handle_t handle,
                               uint8_t endpoint,
                               uint8_t* data,
                               size_t len,
                               int32_t timeout_ms,
                               size_t* out_len);

#ifdef __cplusplus
}
#endif
