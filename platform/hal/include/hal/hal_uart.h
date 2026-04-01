/*
 * EmberLite HAL - UART
 */
#pragma once

#include "hal/hal.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct hal_uart_handle_impl* hal_uart_handle_t;

typedef struct {
    const char* path;      /* e.g. "/dev/ttyUSB0" */
    uint32_t baudrate;     /* common Bxxx or custom (e.g. 100000); on Linux arbitrary rates via BOTHER */
    uint8_t data_bits;     /* 5..8 */
    uint8_t stop_bits;     /* 1 or 2 */
    uint8_t parity;        /* 0=None, 1=Odd, 2=Even */
    uint8_t flow_control;  /* 0=None, 1=HW(RTS/CTS), 2=SW(XON/XOFF) */
} hal_uart_config_t;

hal_status_t hal_uart_open(const hal_uart_config_t* config, hal_uart_handle_t* out_handle);
hal_status_t hal_uart_close(hal_uart_handle_t handle);

hal_status_t hal_uart_read(hal_uart_handle_t handle,
                           uint8_t* buffer,
                           size_t length,
                           int32_t timeout_ms,
                           size_t* out_read_len);

hal_status_t hal_uart_write(hal_uart_handle_t handle,
                            const uint8_t* buffer,
                            size_t length,
                            int32_t timeout_ms,
                            size_t* out_write_len);

hal_status_t hal_uart_set_baudrate(hal_uart_handle_t handle, uint32_t baudrate);
hal_status_t hal_uart_flush(hal_uart_handle_t handle);
int hal_uart_get_fd(hal_uart_handle_t handle);

#ifdef __cplusplus
}
#endif

