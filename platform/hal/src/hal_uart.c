/*
 * EmberLite HAL - UART (POSIX termios)
 */

#include "hal/hal_uart.h"

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>
#include <termios.h>
#include <unistd.h>

struct hal_uart_handle_impl {
    int fd;
    hal_uart_config_t cfg;
    struct termios tio_saved;
    int has_saved;
    pthread_mutex_t mu;
};

static int hal_uart_set_cloexec(int fd)
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

static speed_t hal_uart_baud_to_speed(uint32_t baud)
{
    switch (baud) {
    case 0:
        return B0;
    case 50:
        return B50;
    case 75:
        return B75;
    case 110:
        return B110;
    case 134:
        return B134;
    case 150:
        return B150;
    case 200:
        return B200;
    case 300:
        return B300;
    case 600:
        return B600;
    case 1200:
        return B1200;
    case 1800:
        return B1800;
    case 2400:
        return B2400;
    case 4800:
        return B4800;
    case 9600:
        return B9600;
    case 19200:
        return B19200;
    case 38400:
        return B38400;
#ifdef B57600
    case 57600:
        return B57600;
#endif
#ifdef B115200
    case 115200:
        return B115200;
#endif
#ifdef B230400
    case 230400:
        return B230400;
#endif
#ifdef B460800
    case 460800:
        return B460800;
#endif
#ifdef B500000
    case 500000:
        return B500000;
#endif
#ifdef B576000
    case 576000:
        return B576000;
#endif
#ifdef B921600
    case 921600:
        return B921600;
#endif
    default:
        return (speed_t)0;
    }
}

static hal_status_t hal_uart_apply_config_locked(struct hal_uart_handle_impl* h,
                                                 const hal_uart_config_t* cfg,
                                                 int keep_saved)
{
    struct termios tio;
    if (tcgetattr(h->fd, &tio) != 0) {
        return hal_status_from_errno(errno);
    }

    if (!keep_saved && !h->has_saved) {
        h->tio_saved = tio;
        h->has_saved = 1;
    }

    /*
     * cfmakeraw() is not in ISO C; availability depends on feature macros.
     * Provide a small local equivalent.
     */
    tio.c_iflag &= (tcflag_t)~(IGNBRK | BRKINT | PARMRK | ISTRIP | INLCR | IGNCR | ICRNL | IXON);
    tio.c_oflag &= (tcflag_t)~OPOST;
    tio.c_lflag &= (tcflag_t)~(ECHO | ECHONL | ICANON | ISIG | IEXTEN);
    tio.c_cflag &= (tcflag_t)~(CSIZE | PARENB);
    tio.c_cflag |= CS8;

    /* Enable receiver, ignore modem control lines. */
    tio.c_cflag |= (CLOCAL | CREAD);

    /* Data bits. */
    tio.c_cflag &= (tcflag_t)~CSIZE;
    switch (cfg->data_bits) {
    case 5:
        tio.c_cflag |= CS5;
        break;
    case 6:
        tio.c_cflag |= CS6;
        break;
    case 7:
        tio.c_cflag |= CS7;
        break;
    case 8:
        tio.c_cflag |= CS8;
        break;
    default:
        return HAL_ERR_INVALID_PARAM;
    }

    /* Stop bits. */
    if (cfg->stop_bits == 1) {
        tio.c_cflag &= (tcflag_t)~CSTOPB;
    } else if (cfg->stop_bits == 2) {
        tio.c_cflag |= CSTOPB;
    } else {
        return HAL_ERR_INVALID_PARAM;
    }

    /* Parity. */
    if (cfg->parity == 0) {
        tio.c_cflag &= (tcflag_t)~PARENB;
        tio.c_iflag &= (tcflag_t)~(INPCK | ISTRIP);
    } else if (cfg->parity == 1) {
        tio.c_cflag |= (PARENB | PARODD);
        tio.c_iflag |= INPCK;
    } else if (cfg->parity == 2) {
        tio.c_cflag |= PARENB;
        tio.c_cflag &= (tcflag_t)~PARODD;
        tio.c_iflag |= INPCK;
    } else {
        return HAL_ERR_INVALID_PARAM;
    }

    /* Flow control. */
    if (cfg->flow_control == 0) {
#ifdef CRTSCTS
        tio.c_cflag &= (tcflag_t)~CRTSCTS;
#endif
        tio.c_iflag &= (tcflag_t)~(IXON | IXOFF | IXANY);
    } else if (cfg->flow_control == 1) {
#ifdef CRTSCTS
        tio.c_cflag |= CRTSCTS;
#else
        return HAL_ERR_NOT_SUPPORTED;
#endif
        tio.c_iflag &= (tcflag_t)~(IXON | IXOFF | IXANY);
    } else if (cfg->flow_control == 2) {
#ifdef CRTSCTS
        tio.c_cflag &= (tcflag_t)~CRTSCTS;
#endif
        tio.c_iflag |= (IXON | IXOFF);
        tio.c_iflag &= (tcflag_t)~IXANY;
    } else {
        return HAL_ERR_INVALID_PARAM;
    }

    /* Baud rate. */
    speed_t spd = hal_uart_baud_to_speed(cfg->baudrate);
    if (spd == (speed_t)0) {
        return HAL_ERR_NOT_SUPPORTED;
    }
    if (cfsetispeed(&tio, spd) != 0 || cfsetospeed(&tio, spd) != 0) {
        return hal_status_from_errno(errno);
    }

    /*
     * Make read return as soon as data is available (VMIN=1),
     * and let higher level handle timeouts via poll().
     */
    tio.c_cc[VMIN] = 1;
    tio.c_cc[VTIME] = 0;

    if (tcsetattr(h->fd, TCSANOW, &tio) != 0) {
        return hal_status_from_errno(errno);
    }

    h->cfg = *cfg;
    return HAL_OK;
}

hal_status_t hal_uart_open(const hal_uart_config_t* config, hal_uart_handle_t* out_handle)
{
    if (!config || !out_handle || !config->path) {
        return HAL_ERR_INVALID_PARAM;
    }
    if (config->baudrate == 0) {
        return HAL_ERR_INVALID_PARAM;
    }

    int fd;
#ifdef O_CLOEXEC
    fd = open(config->path, O_RDWR | O_NOCTTY | O_CLOEXEC);
#else
    fd = open(config->path, O_RDWR | O_NOCTTY);
#endif
    if (fd < 0) {
        return hal_status_from_errno(errno);
    }

#ifndef O_CLOEXEC
    (void)hal_uart_set_cloexec(fd);
#endif

    struct hal_uart_handle_impl* h = (struct hal_uart_handle_impl*)calloc(1, sizeof(*h));
    if (!h) {
        close(fd);
        return HAL_ERR_NO_MEMORY;
    }

    h->fd = fd;
    h->has_saved = 0;
    pthread_mutex_init(&h->mu, NULL);

    hal_status_t st;
    pthread_mutex_lock(&h->mu);
    st = hal_uart_apply_config_locked(h, config, 0);
    pthread_mutex_unlock(&h->mu);

    if (st != HAL_OK) {
        (void)close(fd);
        pthread_mutex_destroy(&h->mu);
        free(h);
        return st;
    }

    *out_handle = h;
    return HAL_OK;
}

hal_status_t hal_uart_close(hal_uart_handle_t handle)
{
    if (!handle) {
        return HAL_ERR_INVALID_PARAM;
    }

    struct hal_uart_handle_impl* h = (struct hal_uart_handle_impl*)handle;

    pthread_mutex_lock(&h->mu);
    if (h->fd >= 0 && h->has_saved) {
        (void)tcsetattr(h->fd, TCSANOW, &h->tio_saved);
    }
    int fd = h->fd;
    h->fd = -1;
    pthread_mutex_unlock(&h->mu);

    if (fd >= 0) {
        if (close(fd) != 0) {
            pthread_mutex_destroy(&h->mu);
            free(h);
            return hal_status_from_errno(errno);
        }
    }

    pthread_mutex_destroy(&h->mu);
    free(h);
    return HAL_OK;
}

static hal_status_t hal_uart_wait_fd(int fd, short events, int32_t timeout_ms)
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

hal_status_t hal_uart_read(hal_uart_handle_t handle,
                           uint8_t* buffer,
                           size_t length,
                           int32_t timeout_ms,
                           size_t* out_read_len)
{
    if (!handle || (!buffer && length > 0)) {
        return HAL_ERR_INVALID_PARAM;
    }

    struct hal_uart_handle_impl* h = (struct hal_uart_handle_impl*)handle;

    if (out_read_len) {
        *out_read_len = 0;
    }
    if (length == 0) {
        return HAL_OK;
    }

    pthread_mutex_lock(&h->mu);
    int fd = h->fd;
    pthread_mutex_unlock(&h->mu);
    if (fd < 0) {
        return HAL_ERR_IO;
    }

    hal_status_t st = hal_uart_wait_fd(fd, POLLIN, timeout_ms);
    if (st != HAL_OK) {
        return st;
    }

    ssize_t r;
    do {
        r = read(fd, buffer, length);
    } while (r < 0 && errno == EINTR);

    if (r < 0) {
        return hal_status_from_errno(errno);
    }
    if (out_read_len) {
        *out_read_len = (size_t)r;
    }
    return HAL_OK;
}

hal_status_t hal_uart_write(hal_uart_handle_t handle,
                            const uint8_t* buffer,
                            size_t length,
                            int32_t timeout_ms,
                            size_t* out_write_len)
{
    if (!handle || (!buffer && length > 0)) {
        return HAL_ERR_INVALID_PARAM;
    }

    struct hal_uart_handle_impl* h = (struct hal_uart_handle_impl*)handle;

    if (out_write_len) {
        *out_write_len = 0;
    }
    if (length == 0) {
        return HAL_OK;
    }

    pthread_mutex_lock(&h->mu);
    int fd = h->fd;
    pthread_mutex_unlock(&h->mu);
    if (fd < 0) {
        return HAL_ERR_IO;
    }

    size_t written = 0;
    while (written < length) {
        hal_status_t st = hal_uart_wait_fd(fd, POLLOUT, timeout_ms);
        if (st != HAL_OK) {
            if (out_write_len) {
                *out_write_len = written;
            }
            return st;
        }

        ssize_t w;
        do {
            w = write(fd, buffer + written, length - written);
        } while (w < 0 && errno == EINTR);

        if (w < 0) {
            if (out_write_len) {
                *out_write_len = written;
            }
            return hal_status_from_errno(errno);
        }
        written += (size_t)w;

        /* For finite timeout, only guarantee "some progress" semantics. */
        if (timeout_ms >= 0) {
            break;
        }
    }

    if (out_write_len) {
        *out_write_len = written;
    }
    return HAL_OK;
}

hal_status_t hal_uart_set_baudrate(hal_uart_handle_t handle, uint32_t baudrate)
{
    if (!handle || baudrate == 0) {
        return HAL_ERR_INVALID_PARAM;
    }

    struct hal_uart_handle_impl* h = (struct hal_uart_handle_impl*)handle;

    pthread_mutex_lock(&h->mu);
    if (h->fd < 0) {
        pthread_mutex_unlock(&h->mu);
        return HAL_ERR_IO;
    }

    hal_uart_config_t cfg = h->cfg;
    cfg.baudrate = baudrate;
    hal_status_t st = hal_uart_apply_config_locked(h, &cfg, 1);
    pthread_mutex_unlock(&h->mu);
    return st;
}

hal_status_t hal_uart_flush(hal_uart_handle_t handle)
{
    if (!handle) {
        return HAL_ERR_INVALID_PARAM;
    }
    struct hal_uart_handle_impl* h = (struct hal_uart_handle_impl*)handle;

    pthread_mutex_lock(&h->mu);
    int fd = h->fd;
    pthread_mutex_unlock(&h->mu);

    if (fd < 0) {
        return HAL_ERR_IO;
    }
    if (tcflush(fd, TCIOFLUSH) != 0) {
        return hal_status_from_errno(errno);
    }
    return HAL_OK;
}

int hal_uart_get_fd(hal_uart_handle_t handle)
{
    if (!handle) {
        return -1;
    }
    struct hal_uart_handle_impl* h = (struct hal_uart_handle_impl*)handle;

    pthread_mutex_lock(&h->mu);
    int fd = h->fd;
    pthread_mutex_unlock(&h->mu);
    return fd;
}

