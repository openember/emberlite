/*
 * EmberLite HAL - CAN (Linux SocketCAN)
 */

#include "hal/hal_can.h"

#include <errno.h>
#include <linux/can.h>
#include <linux/can/raw.h>
#include <net/if.h>
#include <poll.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

struct hal_can_handle_impl {
    int fd;
    pthread_mutex_t mu;
};

static hal_status_t wait_fd(int fd, short events, int32_t timeout_ms)
{
    struct pollfd pfd;
    memset(&pfd, 0, sizeof(pfd));
    pfd.fd = fd;
    pfd.events = events;

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

hal_status_t hal_can_open(const hal_can_config_t* config, hal_can_handle_t* out_handle)
{
    if (!config || !out_handle || !config->ifname) {
        return HAL_ERR_INVALID_PARAM;
    }

    int fd = socket(PF_CAN, SOCK_RAW, CAN_RAW);
    if (fd < 0) {
        return hal_status_from_errno(errno);
    }

    struct ifreq ifr;
    memset(&ifr, 0, sizeof(ifr));
    snprintf(ifr.ifr_name, IFNAMSIZ, "%s", config->ifname);
    if (ioctl(fd, SIOCGIFINDEX, &ifr) < 0) {
        hal_status_t st = hal_status_from_errno(errno);
        (void)close(fd);
        return st;
    }

    struct sockaddr_can addr;
    memset(&addr, 0, sizeof(addr));
    addr.can_family = AF_CAN;
    addr.can_ifindex = ifr.ifr_ifindex;
    if (bind(fd, (struct sockaddr*)&addr, sizeof(addr)) < 0) {
        hal_status_t st = hal_status_from_errno(errno);
        (void)close(fd);
        return st;
    }

    /* Optional single-ID filter. */
    if (config->filter_id != 0) {
        struct can_filter flt;
        flt.can_id = config->filter_id;
        flt.can_mask = CAN_SFF_MASK;
        if (setsockopt(fd, SOL_CAN_RAW, CAN_RAW_FILTER, &flt, sizeof(flt)) < 0) {
            hal_status_t st = hal_status_from_errno(errno);
            (void)close(fd);
            return st;
        }
    }

    struct hal_can_handle_impl* h = (struct hal_can_handle_impl*)calloc(1, sizeof(*h));
    if (!h) {
        (void)close(fd);
        return HAL_ERR_NO_MEMORY;
    }
    h->fd = fd;
    pthread_mutex_init(&h->mu, NULL);
    *out_handle = h;
    return HAL_OK;
}

hal_status_t hal_can_close(hal_can_handle_t handle)
{
    if (!handle) {
        return HAL_ERR_INVALID_PARAM;
    }
    struct hal_can_handle_impl* h = (struct hal_can_handle_impl*)handle;

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

hal_status_t hal_can_send(hal_can_handle_t handle, const hal_can_frame_t* frame)
{
    if (!handle || !frame) {
        return HAL_ERR_INVALID_PARAM;
    }
    if (frame->dlc > 8) {
        return HAL_ERR_INVALID_PARAM;
    }

    struct hal_can_handle_impl* h = (struct hal_can_handle_impl*)handle;
    pthread_mutex_lock(&h->mu);
    int fd = h->fd;
    pthread_mutex_unlock(&h->mu);
    if (fd < 0) {
        return HAL_ERR_IO;
    }

    struct can_frame cf;
    memset(&cf, 0, sizeof(cf));
    cf.can_id = frame->id & CAN_SFF_MASK;
    if (frame->is_rtr) {
        cf.can_id |= CAN_RTR_FLAG;
    }
    cf.can_dlc = frame->dlc;
    memcpy(cf.data, frame->data, frame->dlc);

    ssize_t w = write(fd, &cf, sizeof(cf));
    if (w < 0) {
        return hal_status_from_errno(errno);
    }
    if (w != (ssize_t)sizeof(cf)) {
        return HAL_ERR_IO;
    }
    return HAL_OK;
}

hal_status_t hal_can_receive(hal_can_handle_t handle, hal_can_frame_t* frame, int32_t timeout_ms)
{
    if (!handle || !frame) {
        return HAL_ERR_INVALID_PARAM;
    }

    struct hal_can_handle_impl* h = (struct hal_can_handle_impl*)handle;
    pthread_mutex_lock(&h->mu);
    int fd = h->fd;
    pthread_mutex_unlock(&h->mu);
    if (fd < 0) {
        return HAL_ERR_IO;
    }

    hal_status_t st = wait_fd(fd, POLLIN, timeout_ms);
    if (st != HAL_OK) {
        return st;
    }

    struct can_frame cf;
    ssize_t r = read(fd, &cf, sizeof(cf));
    if (r < 0) {
        return hal_status_from_errno(errno);
    }
    if (r != (ssize_t)sizeof(cf)) {
        return HAL_ERR_IO;
    }

    frame->id = cf.can_id & CAN_SFF_MASK;
    frame->is_rtr = (cf.can_id & CAN_RTR_FLAG) ? 1 : 0;
    frame->dlc = cf.can_dlc;
    memset(frame->data, 0, sizeof(frame->data));
    memcpy(frame->data, cf.data, cf.can_dlc <= 8 ? cf.can_dlc : 8);
    return HAL_OK;
}

int hal_can_get_fd(hal_can_handle_t handle)
{
    if (!handle) {
        return -1;
    }
    struct hal_can_handle_impl* h = (struct hal_can_handle_impl*)handle;
    pthread_mutex_lock(&h->mu);
    int fd = h->fd;
    pthread_mutex_unlock(&h->mu);
    return fd;
}

