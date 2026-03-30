/*
 * EmberLite HAL - SPI (Linux spidev)
 */

#include "hal/hal_spi.h"

#include <errno.h>
#include <fcntl.h>
#include <linux/spi/spidev.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

struct hal_spi_handle_impl {
    int fd;
    hal_spi_config_t cfg;
    pthread_mutex_t mu;
};

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

hal_status_t hal_spi_open(const hal_spi_config_t* config, hal_spi_handle_t* out_handle)
{
    if (!config || !out_handle || !config->path) {
        return HAL_ERR_INVALID_PARAM;
    }
    if (config->mode > 3) {
        return HAL_ERR_INVALID_PARAM;
    }
    if (config->bits_per_word == 0) {
        return HAL_ERR_INVALID_PARAM;
    }
    if (config->speed_hz == 0) {
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

    struct hal_spi_handle_impl* h = (struct hal_spi_handle_impl*)calloc(1, sizeof(*h));
    if (!h) {
        (void)close(fd);
        return HAL_ERR_NO_MEMORY;
    }

    h->fd = fd;
    h->cfg = *config;
    pthread_mutex_init(&h->mu, NULL);

    uint8_t mode = config->mode;
    uint8_t bits = config->bits_per_word;
    uint32_t speed = config->speed_hz;

    if (ioctl(fd, SPI_IOC_WR_MODE, &mode) < 0) {
        hal_status_t st = hal_status_from_errno(errno);
        (void)hal_spi_close(h);
        return st;
    }
    if (ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, &bits) < 0) {
        hal_status_t st = hal_status_from_errno(errno);
        (void)hal_spi_close(h);
        return st;
    }
    if (ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, &speed) < 0) {
        hal_status_t st = hal_status_from_errno(errno);
        (void)hal_spi_close(h);
        return st;
    }

    *out_handle = h;
    return HAL_OK;
}

hal_status_t hal_spi_close(hal_spi_handle_t handle)
{
    if (!handle) {
        return HAL_ERR_INVALID_PARAM;
    }
    struct hal_spi_handle_impl* h = (struct hal_spi_handle_impl*)handle;

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

hal_status_t hal_spi_transfer(hal_spi_handle_t handle,
                              const uint8_t* tx,
                              uint8_t* rx,
                              size_t len)
{
    if (!handle) {
        return HAL_ERR_INVALID_PARAM;
    }
    if (len > 0 && !tx && !rx) {
        return HAL_ERR_INVALID_PARAM;
    }
    if (len == 0) {
        return HAL_OK;
    }

    struct hal_spi_handle_impl* h = (struct hal_spi_handle_impl*)handle;

    pthread_mutex_lock(&h->mu);
    int fd = h->fd;
    pthread_mutex_unlock(&h->mu);
    if (fd < 0) {
        return HAL_ERR_IO;
    }

    struct spi_ioc_transfer t;
    memset(&t, 0, sizeof(t));
    t.tx_buf = (unsigned long)tx;
    t.rx_buf = (unsigned long)rx;
    t.len = (uint32_t)len;
    t.speed_hz = h->cfg.speed_hz;
    t.bits_per_word = h->cfg.bits_per_word;

    if (ioctl(fd, SPI_IOC_MESSAGE(1), &t) < 0) {
        return hal_status_from_errno(errno);
    }

    return HAL_OK;
}

int hal_spi_get_fd(hal_spi_handle_t handle)
{
    if (!handle) {
        return -1;
    }
    struct hal_spi_handle_impl* h = (struct hal_spi_handle_impl*)handle;
    pthread_mutex_lock(&h->mu);
    int fd = h->fd;
    pthread_mutex_unlock(&h->mu);
    return fd;
}

