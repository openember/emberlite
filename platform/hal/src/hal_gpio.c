/*
 * EmberLite HAL - GPIO (Linux sysfs)
 */

#include "hal/hal_gpio.h"

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

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

struct hal_gpio_handle_impl {
    uint32_t pin;
    int fd_value;
    uint8_t direction;
    pthread_mutex_t mu;
};

static int write_text_file(const char* path, const char* s)
{
    int fd;
#ifdef O_CLOEXEC
    fd = open(path, O_WRONLY | O_CLOEXEC);
#else
    fd = open(path, O_WRONLY);
#endif
    if (fd < 0) {
        return -1;
    }
#ifndef O_CLOEXEC
    (void)set_cloexec(fd);
#endif
    size_t len = strlen(s);
    ssize_t w = write(fd, s, len);
    int saved = errno;
    (void)close(fd);
    errno = saved;
    return (w == (ssize_t)len) ? 0 : -1;
}

static hal_status_t gpio_export(uint32_t pin)
{
    char buf[32];
    snprintf(buf, sizeof(buf), "%u", pin);
    if (write_text_file("/sys/class/gpio/export", buf) != 0) {
        /* If already exported, ignore EBUSY. */
        if (errno == EBUSY) {
            return HAL_OK;
        }
        return hal_status_from_errno(errno);
    }
    return HAL_OK;
}

static hal_status_t gpio_set_direction(uint32_t pin, uint8_t direction)
{
    char path[128];
    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%u/direction", pin);
    if (direction == 0) {
        if (write_text_file(path, "in") != 0) {
            return hal_status_from_errno(errno);
        }
        return HAL_OK;
    }
    if (direction == 1) {
        if (write_text_file(path, "out") != 0) {
            return hal_status_from_errno(errno);
        }
        return HAL_OK;
    }
    return HAL_ERR_INVALID_PARAM;
}

hal_status_t hal_gpio_open(const hal_gpio_config_t* config, hal_gpio_handle_t* out_handle)
{
    if (!config || !out_handle) {
        return HAL_ERR_INVALID_PARAM;
    }

    if (config->pull_mode != 0) {
        return HAL_ERR_NOT_SUPPORTED;
    }
    if (config->direction != 0 && config->direction != 1) {
        return HAL_ERR_INVALID_PARAM;
    }

    hal_status_t st = gpio_export(config->pin);
    if (st != HAL_OK) {
        return st;
    }

    st = gpio_set_direction(config->pin, config->direction);
    if (st != HAL_OK) {
        return st;
    }

    char path[128];
    snprintf(path, sizeof(path), "/sys/class/gpio/gpio%u/value", config->pin);

    int fd;
#ifdef O_CLOEXEC
    fd = open(path, O_RDWR | O_CLOEXEC);
#else
    fd = open(path, O_RDWR);
#endif
    if (fd < 0) {
        return hal_status_from_errno(errno);
    }
#ifndef O_CLOEXEC
    (void)set_cloexec(fd);
#endif

    struct hal_gpio_handle_impl* h = (struct hal_gpio_handle_impl*)calloc(1, sizeof(*h));
    if (!h) {
        (void)close(fd);
        return HAL_ERR_NO_MEMORY;
    }

    h->pin = config->pin;
    h->fd_value = fd;
    h->direction = config->direction;
    pthread_mutex_init(&h->mu, NULL);

    if (config->direction == 1) {
        (void)hal_gpio_write(h, config->initial_level ? 1 : 0);
    }

    *out_handle = h;
    return HAL_OK;
}

hal_status_t hal_gpio_close(hal_gpio_handle_t handle)
{
    if (!handle) {
        return HAL_ERR_INVALID_PARAM;
    }
    struct hal_gpio_handle_impl* h = (struct hal_gpio_handle_impl*)handle;

    pthread_mutex_lock(&h->mu);
    int fd = h->fd_value;
    h->fd_value = -1;
    pthread_mutex_unlock(&h->mu);

    if (fd >= 0) {
        (void)close(fd);
    }

    pthread_mutex_destroy(&h->mu);
    free(h);
    return HAL_OK;
}

hal_status_t hal_gpio_read(hal_gpio_handle_t handle, uint8_t* out_level)
{
    if (!handle || !out_level) {
        return HAL_ERR_INVALID_PARAM;
    }
    struct hal_gpio_handle_impl* h = (struct hal_gpio_handle_impl*)handle;

    pthread_mutex_lock(&h->mu);
    int fd = h->fd_value;
    pthread_mutex_unlock(&h->mu);
    if (fd < 0) {
        return HAL_ERR_IO;
    }

    char c = 0;
    if (lseek(fd, 0, SEEK_SET) < 0) {
        return hal_status_from_errno(errno);
    }
    ssize_t r = read(fd, &c, 1);
    if (r < 0) {
        return hal_status_from_errno(errno);
    }
    *out_level = (c == '1') ? 1 : 0;
    return HAL_OK;
}

hal_status_t hal_gpio_write(hal_gpio_handle_t handle, uint8_t level)
{
    if (!handle) {
        return HAL_ERR_INVALID_PARAM;
    }
    struct hal_gpio_handle_impl* h = (struct hal_gpio_handle_impl*)handle;
    if (h->direction != 1) {
        return HAL_ERR_INVALID_PARAM;
    }

    pthread_mutex_lock(&h->mu);
    int fd = h->fd_value;
    pthread_mutex_unlock(&h->mu);
    if (fd < 0) {
        return HAL_ERR_IO;
    }

    const char c = level ? '1' : '0';
    if (lseek(fd, 0, SEEK_SET) < 0) {
        return hal_status_from_errno(errno);
    }
    ssize_t w = write(fd, &c, 1);
    if (w < 0) {
        return hal_status_from_errno(errno);
    }
    return HAL_OK;
}

hal_status_t hal_gpio_toggle(hal_gpio_handle_t handle)
{
    uint8_t v = 0;
    hal_status_t st = hal_gpio_read(handle, &v);
    if (st != HAL_OK) {
        return st;
    }
    return hal_gpio_write(handle, v ? 0 : 1);
}

int hal_gpio_get_fd(hal_gpio_handle_t handle)
{
    if (!handle) {
        return -1;
    }
    struct hal_gpio_handle_impl* h = (struct hal_gpio_handle_impl*)handle;
    pthread_mutex_lock(&h->mu);
    int fd = h->fd_value;
    pthread_mutex_unlock(&h->mu);
    return fd;
}

