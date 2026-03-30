/*
 * EmberLite HAL - Core types
 */
#pragma once

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    HAL_OK                = 0,
    HAL_ERR_IO            = 1,
    HAL_ERR_TIMEOUT       = 2,
    HAL_ERR_INVALID_PARAM = 3,
    HAL_ERR_NOT_SUPPORTED = 4,
    HAL_ERR_NO_MEMORY     = 5,
    HAL_ERR_BUSY          = 6,
    HAL_ERR_PERMISSION    = 7,
    HAL_ERR_NODEVICE      = 8
} hal_status_t;

const char* hal_status_str(hal_status_t s);
hal_status_t hal_status_from_errno(int err);

#ifdef __cplusplus
}
#endif

