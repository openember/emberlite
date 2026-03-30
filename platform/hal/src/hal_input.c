/*
 * EmberLite HAL - Input (Linux evdev)
 */

#include "hal/hal_input.h"

#include <errno.h>
#include <fcntl.h>
#include <linux/input.h>
#include <poll.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct hal_input_handle_impl {
    int fd;
    pthread_mutex_t mu;
};

hal_status_t hal_input_open(const hal_input_config_t* config, hal_input_handle_t* out_handle)
{
    if (!config || !out_handle || !config->device_path) {
        return HAL_ERR_INVALID_PARAM;
    }

    int fd;
#ifdef O_CLOEXEC
    fd = open(config->device_path, O_RDONLY | O_CLOEXEC | O_NONBLOCK);
#else
    fd = open(config->device_path, O_RDONLY | O_NONBLOCK);
#endif
    if (fd < 0) {
        return hal_status_from_errno(errno);
    }

    struct hal_input_handle_impl* h = (struct hal_input_handle_impl*)calloc(1, sizeof(*h));
    if (!h) {
        (void)close(fd);
        return HAL_ERR_NO_MEMORY;
    }
    h->fd = fd;
    pthread_mutex_init(&h->mu, NULL);
    *out_handle = h;
    return HAL_OK;
}

hal_status_t hal_input_close(hal_input_handle_t handle)
{
    if (!handle) {
        return HAL_ERR_INVALID_PARAM;
    }
    struct hal_input_handle_impl* h = (struct hal_input_handle_impl*)handle;

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

static hal_status_t wait_fd(int fd, int32_t timeout_ms)
{
    struct pollfd pfd;
    memset(&pfd, 0, sizeof(pfd));
    pfd.fd = fd;
    pfd.events = POLLIN;

    int tmo = timeout_ms;
    if (timeout_ms < 0) {
        tmo = -1;
    }

    int rc;
    do {
        rc = poll(&pfd, 1, tmo);
    } while (rc < 0 && errno == EINTR);

    if (rc == 0) {
        return HAL_ERR_TIMEOUT;
    }
    if (rc < 0) {
        return hal_status_from_errno(errno);
    }
    if (pfd.revents & (POLLERR | POLLHUP | POLLNVAL)) {
        return HAL_ERR_IO;
    }
    return HAL_OK;
}

hal_status_t hal_input_read_event(hal_input_handle_t handle,
                                  hal_input_event_t* out_event,
                                  int32_t timeout_ms)
{
    if (!handle || !out_event) {
        return HAL_ERR_INVALID_PARAM;
    }
    struct hal_input_handle_impl* h = (struct hal_input_handle_impl*)handle;

    pthread_mutex_lock(&h->mu);
    int fd = h->fd;
    pthread_mutex_unlock(&h->mu);
    if (fd < 0) {
        return HAL_ERR_IO;
    }

    if (timeout_ms != 0) {
        hal_status_t st = wait_fd(fd, timeout_ms);
        if (st != HAL_OK) {
            return st;
        }
    }

    struct input_event ev;
    ssize_t r;
    do {
        r = read(fd, &ev, sizeof(ev));
    } while (r < 0 && errno == EINTR);

    if (r < 0) {
        if (errno == EAGAIN) {
            return HAL_ERR_TIMEOUT;
        }
        return hal_status_from_errno(errno);
    }
    if (r != (ssize_t)sizeof(ev)) {
        return HAL_ERR_IO;
    }

    out_event->time_sec = (uint64_t)ev.time.tv_sec;
    out_event->time_usec = (uint64_t)ev.time.tv_usec;
    out_event->type = ev.type;
    out_event->code = ev.code;
    out_event->value = ev.value;
    return HAL_OK;
}

int hal_input_get_fd(hal_input_handle_t handle)
{
    if (!handle) {
        return -1;
    }
    struct hal_input_handle_impl* h = (struct hal_input_handle_impl*)handle;
    pthread_mutex_lock(&h->mu);
    int fd = h->fd;
    pthread_mutex_unlock(&h->mu);
    return fd;
}
