/*
 * EmberLite HAL - USB (libusb-1.0)
 */

#include "hal/hal_usb.h"

#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#if defined(HAL_HAS_LIBUSB) && HAL_HAS_LIBUSB
#include <libusb-1.0/libusb.h>
#endif

struct hal_usb_handle_impl {
#if defined(HAL_HAS_LIBUSB) && HAL_HAS_LIBUSB
    libusb_context* ctx;
    libusb_device_handle* dev;
#endif
    int interface_number;
    pthread_mutex_t mu;
};

#if defined(HAL_HAS_LIBUSB) && HAL_HAS_LIBUSB

hal_status_t hal_usb_open(const hal_usb_config_t* config, hal_usb_handle_t* out_handle)
{
    if (!config || !out_handle) {
        return HAL_ERR_INVALID_PARAM;
    }

    struct hal_usb_handle_impl* h = (struct hal_usb_handle_impl*)calloc(1, sizeof(*h));
    if (!h) {
        return HAL_ERR_NO_MEMORY;
    }
    h->interface_number = config->interface_number;
    pthread_mutex_init(&h->mu, NULL);

    int err = libusb_init(&h->ctx);
    if (err != 0) {
        pthread_mutex_destroy(&h->mu);
        free(h);
        return HAL_ERR_IO;
    }

    h->dev = libusb_open_device_with_vid_pid(h->ctx, config->vendor_id, config->product_id);
    if (!h->dev) {
        libusb_exit(h->ctx);
        pthread_mutex_destroy(&h->mu);
        free(h);
        return HAL_ERR_NODEVICE;
    }

    err = libusb_detach_kernel_driver(h->dev, config->interface_number);
    if (err != 0 && err != LIBUSB_ERROR_NOT_FOUND) {
        libusb_close(h->dev);
        libusb_exit(h->ctx);
        pthread_mutex_destroy(&h->mu);
        free(h);
        return HAL_ERR_IO;
    }

    err = libusb_claim_interface(h->dev, config->interface_number);
    if (err != 0) {
        libusb_close(h->dev);
        libusb_exit(h->ctx);
        pthread_mutex_destroy(&h->mu);
        free(h);
        return HAL_ERR_IO;
    }

    *out_handle = h;
    return HAL_OK;
}

hal_status_t hal_usb_close(hal_usb_handle_t handle)
{
    if (!handle) {
        return HAL_ERR_INVALID_PARAM;
    }
    struct hal_usb_handle_impl* h = (struct hal_usb_handle_impl*)handle;

    pthread_mutex_lock(&h->mu);
    if (h->dev) {
        (void)libusb_release_interface(h->dev, h->interface_number);
        libusb_close(h->dev);
        h->dev = NULL;
    }
    if (h->ctx) {
        libusb_exit(h->ctx);
        h->ctx = NULL;
    }
    pthread_mutex_unlock(&h->mu);
    pthread_mutex_destroy(&h->mu);
    free(h);
    return HAL_OK;
}

hal_status_t hal_usb_control_transfer(hal_usb_handle_t handle,
                                      uint8_t req_type,
                                      uint8_t req,
                                      uint16_t val,
                                      uint16_t idx,
                                      uint8_t* data,
                                      size_t len,
                                      int timeout_ms)
{
    if (!handle) {
        return HAL_ERR_INVALID_PARAM;
    }
    struct hal_usb_handle_impl* h = (struct hal_usb_handle_impl*)handle;

    pthread_mutex_lock(&h->mu);
    libusb_device_handle* dev = h->dev;
    pthread_mutex_unlock(&h->mu);
    if (!dev) {
        return HAL_ERR_IO;
    }

    if (len > 65535u) {
        return HAL_ERR_INVALID_PARAM;
    }
    int tmo = timeout_ms < 0 ? 1000 : timeout_ms;
    int r = libusb_control_transfer(dev, req_type, req, val, idx, data, (uint16_t)len, (unsigned int)tmo);
    if (r < 0) {
        return HAL_ERR_IO;
    }
    return HAL_OK;
}

hal_status_t hal_usb_bulk_write(hal_usb_handle_t handle,
                                uint8_t endpoint,
                                const uint8_t* data,
                                size_t len,
                                int32_t timeout_ms)
{
    if (!handle || (!data && len > 0)) {
        return HAL_ERR_INVALID_PARAM;
    }
    struct hal_usb_handle_impl* h = (struct hal_usb_handle_impl*)handle;

    pthread_mutex_lock(&h->mu);
    libusb_device_handle* dev = h->dev;
    pthread_mutex_unlock(&h->mu);
    if (!dev) {
        return HAL_ERR_IO;
    }

    int transferred = 0;
    int tmo = timeout_ms < 0 ? 60000 : (int)timeout_ms;
    int err = libusb_bulk_transfer(dev, endpoint, (unsigned char*)data, (int)len, &transferred, (unsigned int)tmo);
    if (err != 0) {
        return HAL_ERR_IO;
    }
    if ((size_t)transferred != len) {
        return HAL_ERR_IO;
    }
    return HAL_OK;
}

hal_status_t hal_usb_bulk_read(hal_usb_handle_t handle,
                               uint8_t endpoint,
                               uint8_t* data,
                               size_t len,
                               int32_t timeout_ms,
                               size_t* out_len)
{
    if (!handle || (!data && len > 0)) {
        return HAL_ERR_INVALID_PARAM;
    }
    struct hal_usb_handle_impl* h = (struct hal_usb_handle_impl*)handle;

    if (out_len) {
        *out_len = 0;
    }

    pthread_mutex_lock(&h->mu);
    libusb_device_handle* dev = h->dev;
    pthread_mutex_unlock(&h->mu);
    if (!dev) {
        return HAL_ERR_IO;
    }

    int transferred = 0;
    int tmo = timeout_ms < 0 ? 60000 : (int)timeout_ms;
    int err = libusb_bulk_transfer(dev, endpoint, data, (int)len, &transferred, (unsigned int)tmo);
    if (err != 0) {
        return HAL_ERR_IO;
    }
    if (out_len) {
        *out_len = (size_t)transferred;
    }
    return HAL_OK;
}

#else /* !HAL_HAS_LIBUSB */

hal_status_t hal_usb_open(const hal_usb_config_t* config, hal_usb_handle_t* out_handle)
{
    (void)config;
    (void)out_handle;
    return HAL_ERR_NOT_SUPPORTED;
}

hal_status_t hal_usb_close(hal_usb_handle_t handle)
{
    (void)handle;
    return HAL_ERR_INVALID_PARAM;
}

hal_status_t hal_usb_control_transfer(hal_usb_handle_t handle,
                                      uint8_t req_type,
                                      uint8_t req,
                                      uint16_t val,
                                      uint16_t idx,
                                      uint8_t* data,
                                      size_t len,
                                      int timeout_ms)
{
    (void)handle;
    (void)req_type;
    (void)req;
    (void)val;
    (void)idx;
    (void)data;
    (void)len;
    (void)timeout_ms;
    return HAL_ERR_NOT_SUPPORTED;
}

hal_status_t hal_usb_bulk_write(hal_usb_handle_t handle,
                                uint8_t endpoint,
                                const uint8_t* data,
                                size_t len,
                                int32_t timeout_ms)
{
    (void)handle;
    (void)endpoint;
    (void)data;
    (void)len;
    (void)timeout_ms;
    return HAL_ERR_NOT_SUPPORTED;
}

hal_status_t hal_usb_bulk_read(hal_usb_handle_t handle,
                               uint8_t endpoint,
                               uint8_t* data,
                               size_t len,
                               int32_t timeout_ms,
                               size_t* out_len)
{
    (void)handle;
    (void)endpoint;
    (void)data;
    (void)len;
    (void)timeout_ms;
    (void)out_len;
    return HAL_ERR_NOT_SUPPORTED;
}

#endif
