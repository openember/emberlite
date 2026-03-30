/*
 * EmberLite HAL - 1-Wire (Linux w1-gpio sysfs)
 */

#include "hal/hal_onewire.h"

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct hal_onewire_handle_impl {
    char device_dir[384];
    pthread_mutex_t mu;
};

hal_status_t hal_onewire_open(const hal_onewire_config_t* cfg, hal_onewire_handle_t* out_handle)
{
    if (!cfg || !out_handle || !cfg->bus_path || !cfg->device_id) {
        return HAL_ERR_INVALID_PARAM;
    }

    struct hal_onewire_handle_impl* h = (struct hal_onewire_handle_impl*)calloc(1, sizeof(*h));
    if (!h) {
        return HAL_ERR_NO_MEMORY;
    }

    snprintf(h->device_dir, sizeof(h->device_dir), "%s/%s", cfg->bus_path, cfg->device_id);
    pthread_mutex_init(&h->mu, NULL);
    *out_handle = h;
    return HAL_OK;
}

hal_status_t hal_onewire_close(hal_onewire_handle_t handle)
{
    if (!handle) {
        return HAL_ERR_INVALID_PARAM;
    }
    struct hal_onewire_handle_impl* h = (struct hal_onewire_handle_impl*)handle;
    pthread_mutex_destroy(&h->mu);
    free(h);
    return HAL_OK;
}

hal_status_t hal_onewire_read_raw(hal_onewire_handle_t handle, uint8_t* buf, size_t len, size_t* out_len)
{
    if (!handle || !buf || len == 0) {
        return HAL_ERR_INVALID_PARAM;
    }
    struct hal_onewire_handle_impl* h = (struct hal_onewire_handle_impl*)handle;

    char path[448];
    snprintf(path, sizeof(path), "%s/w1_slave", h->device_dir);

    pthread_mutex_lock(&h->mu);
    int fd = open(path, O_RDONLY | O_CLOEXEC);
    if (fd < 0) {
        pthread_mutex_unlock(&h->mu);
        return hal_status_from_errno(errno);
    }
    ssize_t r = read(fd, buf, len);
    int saved = errno;
    (void)close(fd);
    pthread_mutex_unlock(&h->mu);

    if (r < 0) {
        errno = saved;
        return hal_status_from_errno(errno);
    }
    if (out_len) {
        *out_len = (size_t)r;
    }
    return HAL_OK;
}

hal_status_t hal_onewire_read_temp(hal_onewire_handle_t handle, float* temp_c)
{
    if (!handle || !temp_c) {
        return HAL_ERR_INVALID_PARAM;
    }

    uint8_t buf[512];
    size_t n = 0;
    hal_status_t st = hal_onewire_read_raw(handle, buf, sizeof(buf) - 1, &n);
    if (st != HAL_OK) {
        return st;
    }
    buf[n] = '\0';

    const char* p = strstr((const char*)buf, "t=");
    if (!p) {
        return HAL_ERR_IO;
    }
    p += 2;
    errno = 0;
    char* end = NULL;
    long t_milli = strtol(p, &end, 10);
    if (errno != 0 || end == p) {
        return HAL_ERR_IO;
    }
    *temp_c = (float)t_milli / 1000.0f;
    return HAL_OK;
}
