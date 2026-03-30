/*
 * EmberLite HAL - Core helpers
 */

#include "hal/hal.h"

#include <errno.h>

const char* hal_status_str(hal_status_t s)
{
    switch (s) {
    case HAL_OK:
        return "HAL_OK";
    case HAL_ERR_IO:
        return "HAL_ERR_IO";
    case HAL_ERR_TIMEOUT:
        return "HAL_ERR_TIMEOUT";
    case HAL_ERR_INVALID_PARAM:
        return "HAL_ERR_INVALID_PARAM";
    case HAL_ERR_NOT_SUPPORTED:
        return "HAL_ERR_NOT_SUPPORTED";
    case HAL_ERR_NO_MEMORY:
        return "HAL_ERR_NO_MEMORY";
    case HAL_ERR_BUSY:
        return "HAL_ERR_BUSY";
    case HAL_ERR_PERMISSION:
        return "HAL_ERR_PERMISSION";
    case HAL_ERR_NODEVICE:
        return "HAL_ERR_NODEVICE";
    default:
        return "HAL_ERR_UNKNOWN";
    }
}

hal_status_t hal_status_from_errno(int err)
{
    switch (err) {
    case 0:
        return HAL_OK;
    case ETIMEDOUT:
/* EWOULDBLOCK is often the same value as EAGAIN on Linux. */
#if defined(EWOULDBLOCK) && (!defined(EAGAIN) || (EWOULDBLOCK != EAGAIN))
    case EWOULDBLOCK:
#endif
#if defined(EAGAIN)
    case EAGAIN:
#endif
        return HAL_ERR_TIMEOUT;
    case EINVAL:
        return HAL_ERR_INVALID_PARAM;
/* ENOTSUP may alias EOPNOTSUPP. */
#if defined(ENOTSUP) && (!defined(EOPNOTSUPP) || (ENOTSUP != EOPNOTSUPP))
    case ENOTSUP:
#endif
#if defined(EOPNOTSUPP)
    case EOPNOTSUPP:
#endif
        return HAL_ERR_NOT_SUPPORTED;
    case ENOMEM:
        return HAL_ERR_NO_MEMORY;
    case EBUSY:
        return HAL_ERR_BUSY;
    case EACCES:
    case EPERM:
        return HAL_ERR_PERMISSION;
    case ENODEV:
    case ENOENT:
        return HAL_ERR_NODEVICE;
    default:
        return HAL_ERR_IO;
    }
}

