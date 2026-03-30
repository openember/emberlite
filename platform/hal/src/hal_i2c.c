/*
 * EmberLite HAL - I2C (Linux i2c-dev)
 */

#include "hal/hal_i2c.h"

#include <errno.h>
#include <fcntl.h>
#include <linux/i2c-dev.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

struct hal_i2c_handle_impl {
    int fd;
    hal_i2c_config_t cfg;
    pthread_mutex_t mu;
};

#ifndef O_CLOEXEC
static int set_cloexec(int fd)
{
    int flags = fcntl(fd, F_GETFD);
    if (flags < 0) {
        return -1;
    }
    if (fcntl(fd, F_SETFD, flags | FD_CLOEXEC) < 0) {
        return -1;
    }
    return 0;
}
#endif

static hal_status_t set_slave(int fd, uint16_t addr)
{
    if (addr > 0x7fu) {
        return HAL_ERR_INVALID_PARAM;
    }
    if (ioctl(fd, I2C_SLAVE, (unsigned long)addr) < 0) {
        return hal_status_from_errno(errno);
    }
    return HAL_OK;
}

hal_status_t hal_i2c_open(const hal_i2c_config_t* config, hal_i2c_handle_t* out_handle)
{
    if (!config || !out_handle || !config->path) {
        return HAL_ERR_INVALID_PARAM;
    }

    int fd;
#ifdef O_CLOEXEC
    fd = open(config->path, O_RDWR | O_CLOEXEC);
#else
    fd = open(config->path, O_RDWR);
#endif
    if (fd < 0) {
        return hal_status_from_errno(errno);
    }
#ifndef O_CLOEXEC
    (void)set_cloexec(fd);
#endif

    hal_status_t st = set_slave(fd, config->slave_addr);
    if (st != HAL_OK) {
        (void)close(fd);
        return st;
    }

    struct hal_i2c_handle_impl* h = (struct hal_i2c_handle_impl*)calloc(1, sizeof(*h));
    if (!h) {
        (void)close(fd);
        return HAL_ERR_NO_MEMORY;
    }
    h->fd = fd;
    h->cfg = *config;
    pthread_mutex_init(&h->mu, NULL);
    *out_handle = h;
    return HAL_OK;
}

hal_status_t hal_i2c_close(hal_i2c_handle_t handle)
{
    if (!handle) {
        return HAL_ERR_INVALID_PARAM;
    }
    struct hal_i2c_handle_impl* h = (struct hal_i2c_handle_impl*)handle;

    pthread_mutex_lock(&h->mu);
    int fd = h->fd;
    h->fd = -1;
    pthread_mutex_unlock(&h->mu);

    if (fd >= 0) {
        (void)close(fd);
    }
    pthread_mutex_destroy(&h->mu);
    free(h);
    return HAL_OK;
}

hal_status_t hal_i2c_write(hal_i2c_handle_t handle, const uint8_t* data, size_t len)
{
    if (!handle || (!data && len > 0)) {
        return HAL_ERR_INVALID_PARAM;
    }
    if (len == 0) {
        return HAL_OK;
    }
    struct hal_i2c_handle_impl* h = (struct hal_i2c_handle_impl*)handle;

    pthread_mutex_lock(&h->mu);
    int fd = h->fd;
    pthread_mutex_unlock(&h->mu);
    if (fd < 0) {
        return HAL_ERR_IO;
    }

    ssize_t w = write(fd, data, len);
    if (w < 0) {
        return hal_status_from_errno(errno);
    }
    if ((size_t)w != len) {
        return HAL_ERR_IO;
    }
    return HAL_OK;
}

hal_status_t hal_i2c_read(hal_i2c_handle_t handle, uint8_t* data, size_t len)
{
    if (!handle || (!data && len > 0)) {
        return HAL_ERR_INVALID_PARAM;
    }
    if (len == 0) {
        return HAL_OK;
    }
    struct hal_i2c_handle_impl* h = (struct hal_i2c_handle_impl*)handle;

    pthread_mutex_lock(&h->mu);
    int fd = h->fd;
    pthread_mutex_unlock(&h->mu);
    if (fd < 0) {
        return HAL_ERR_IO;
    }

    ssize_t r = read(fd, data, len);
    if (r < 0) {
        return hal_status_from_errno(errno);
    }
    if ((size_t)r != len) {
        return HAL_ERR_IO;
    }
    return HAL_OK;
}

hal_status_t hal_i2c_write_reg(hal_i2c_handle_t handle, uint8_t reg, const uint8_t* data, size_t len)
{
    if (!handle || (!data && len > 0)) {
        return HAL_ERR_INVALID_PARAM;
    }
    struct hal_i2c_handle_impl* h = (struct hal_i2c_handle_impl*)handle;

    pthread_mutex_lock(&h->mu);
    int fd = h->fd;
    pthread_mutex_unlock(&h->mu);
    if (fd < 0) {
        return HAL_ERR_IO;
    }

    uint8_t* tmp = (uint8_t*)malloc(len + 1);
    if (!tmp) {
        return HAL_ERR_NO_MEMORY;
    }
    tmp[0] = reg;
    if (len > 0) {
        memcpy(tmp + 1, data, len);
    }

    ssize_t w = write(fd, tmp, len + 1);
    int saved = errno;
    free(tmp);
    errno = saved;
    if (w < 0) {
        return hal_status_from_errno(errno);
    }
    if ((size_t)w != (len + 1)) {
        return HAL_ERR_IO;
    }
    return HAL_OK;
}

hal_status_t hal_i2c_read_reg(hal_i2c_handle_t handle, uint8_t reg, uint8_t* data, size_t len)
{
    if (!handle || (!data && len > 0)) {
        return HAL_ERR_INVALID_PARAM;
    }
    if (len == 0) {
        return HAL_OK;
    }
    struct hal_i2c_handle_impl* h = (struct hal_i2c_handle_impl*)handle;

    pthread_mutex_lock(&h->mu);
    int fd = h->fd;
    pthread_mutex_unlock(&h->mu);
    if (fd < 0) {
        return HAL_ERR_IO;
    }

    /* Write register pointer, then read data. */
    uint8_t r = reg;
    ssize_t w = write(fd, &r, 1);
    if (w < 0) {
        return hal_status_from_errno(errno);
    }
    if (w != 1) {
        return HAL_ERR_IO;
    }

    ssize_t rr = read(fd, data, len);
    if (rr < 0) {
        return hal_status_from_errno(errno);
    }
    if ((size_t)rr != len) {
        return HAL_ERR_IO;
    }
    return HAL_OK;
}

int hal_i2c_get_fd(hal_i2c_handle_t handle)
{
    if (!handle) {
        return -1;
    }
    struct hal_i2c_handle_impl* h = (struct hal_i2c_handle_impl*)handle;
    pthread_mutex_lock(&h->mu);
    int fd = h->fd;
    pthread_mutex_unlock(&h->mu);
    return fd;
}

