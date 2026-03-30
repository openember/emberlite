/*
 * EmberLite HAL - System time, timerfd timers, watchdog
 */

#include "hal/hal_system.h"

#include <errno.h>
#include <fcntl.h>
#include <linux/watchdog.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/timerfd.h>
#include <time.h>
#include <unistd.h>

uint64_t hal_system_get_time_ms(void)
{
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
        return 0;
    }
    return (uint64_t)ts.tv_sec * 1000ULL + (uint64_t)ts.tv_nsec / 1000000ULL;
}

uint64_t hal_system_get_time_us(void)
{
    struct timespec ts;
    if (clock_gettime(CLOCK_MONOTONIC, &ts) != 0) {
        return 0;
    }
    return (uint64_t)ts.tv_sec * 1000000ULL + (uint64_t)ts.tv_nsec / 1000ULL;
}

uint64_t hal_system_get_uptime_ms(void)
{
    struct timespec ts;
#ifdef CLOCK_BOOTTIME
    if (clock_gettime(CLOCK_BOOTTIME, &ts) == 0) {
        return (uint64_t)ts.tv_sec * 1000ULL + (uint64_t)ts.tv_nsec / 1000000ULL;
    }
#endif
    FILE* f = fopen("/proc/uptime", "r");
    if (!f) {
        return 0;
    }
    double up = 0.0;
    if (fscanf(f, "%lf", &up) != 1) {
        fclose(f);
        return 0;
    }
    fclose(f);
    return (uint64_t)(up * 1000.0 + 0.5);
}

uint64_t hal_system_get_cpu_temp(void)
{
    char buf[64];
    FILE* f = fopen("/sys/class/thermal/thermal_zone0/temp", "r");
    if (!f) {
        return 0;
    }
    if (!fgets(buf, sizeof(buf), f)) {
        fclose(f);
        return 0;
    }
    fclose(f);
    errno = 0;
    char* end = NULL;
    unsigned long v = strtoul(buf, &end, 10);
    if (errno != 0) {
        return 0;
    }
    return (uint64_t)v;
}

void hal_system_sleep_ms(uint32_t ms)
{
    struct timespec ts;
    ts.tv_sec = (time_t)(ms / 1000u);
    ts.tv_nsec = (long)((ms % 1000u) * 1000000u);
    while (nanosleep(&ts, &ts) != 0 && errno == EINTR) {
    }
}

struct hal_timer_impl {
    int fd;
    pthread_t thread;
    int thread_started;
    volatile int stop;
    hal_timer_config_t cfg;
    hal_timer_callback_t cb;
    void* user_data;
    pthread_mutex_t mu;
};

static void* timer_thread_main(void* arg)
{
    struct hal_timer_impl* t = (struct hal_timer_impl*)arg;
    while (!t->stop) {
        uint64_t exp;
        ssize_t r = read(t->fd, &exp, sizeof(exp));
        if (r <= 0) {
            if (errno == EINTR) {
                continue;
            }
            break;
        }
        if (t->cb) {
            t->cb((hal_timer_handle_t)t, t->user_data);
        }
    }
    return NULL;
}

hal_timer_handle_t hal_timer_create(const hal_timer_config_t* cfg,
                                    hal_timer_callback_t cb,
                                    void* user_data)
{
    if (!cfg) {
        return NULL;
    }

    struct hal_timer_impl* t = (struct hal_timer_impl*)calloc(1, sizeof(*t));
    if (!t) {
        return NULL;
    }
    t->cfg = *cfg;
    t->cb = cb;
    t->user_data = user_data;
    pthread_mutex_init(&t->mu, NULL);

    t->fd = timerfd_create(CLOCK_MONOTONIC, TFD_CLOEXEC);
    if (t->fd < 0) {
        pthread_mutex_destroy(&t->mu);
        free(t);
        return NULL;
    }

    if (cb) {
        if (pthread_create(&t->thread, NULL, timer_thread_main, t) != 0) {
            close(t->fd);
            pthread_mutex_destroy(&t->mu);
            free(t);
            return NULL;
        }
        t->thread_started = 1;
    }

    return (hal_timer_handle_t)t;
}

hal_status_t hal_timer_start(hal_timer_handle_t handle)
{
    struct hal_timer_impl* t = (struct hal_timer_impl*)handle;
    if (!t) {
        return HAL_ERR_INVALID_PARAM;
    }

    struct itimerspec its;
    memset(&its, 0, sizeof(its));
    its.it_value.tv_sec = (time_t)(t->cfg.initial_ms / 1000u);
    its.it_value.tv_nsec = (long)((t->cfg.initial_ms % 1000u) * 1000000u);
    if (t->cfg.periodic) {
        its.it_interval.tv_sec = (time_t)(t->cfg.interval_ms / 1000u);
        its.it_interval.tv_nsec = (long)((t->cfg.interval_ms % 1000u) * 1000000u);
    }

    if (timerfd_settime(t->fd, 0, &its, NULL) != 0) {
        return hal_status_from_errno(errno);
    }
    return HAL_OK;
}

hal_status_t hal_timer_stop(hal_timer_handle_t handle)
{
    struct hal_timer_impl* t = (struct hal_timer_impl*)handle;
    if (!t) {
        return HAL_ERR_INVALID_PARAM;
    }

    struct itimerspec its;
    memset(&its, 0, sizeof(its));
    if (timerfd_settime(t->fd, 0, &its, NULL) != 0) {
        return hal_status_from_errno(errno);
    }
    return HAL_OK;
}

void hal_timer_destroy(hal_timer_handle_t handle)
{
    struct hal_timer_impl* t = (struct hal_timer_impl*)handle;
    if (!t) {
        return;
    }

    t->stop = 1;
    if (t->fd >= 0) {
        int fd = t->fd;
        t->fd = -1;
        (void)close(fd);
    }
    if (t->thread_started) {
        (void)pthread_join(t->thread, NULL);
    }
    pthread_mutex_destroy(&t->mu);
    free(t);
}

int hal_timer_get_fd(hal_timer_handle_t handle)
{
    struct hal_timer_impl* t = (struct hal_timer_impl*)handle;
    if (!t) {
        return -1;
    }
    return t->fd;
}

struct hal_watchdog_handle_impl {
    int fd;
    pthread_mutex_t mu;
};

hal_status_t hal_watchdog_open(const char* dev_path, hal_watchdog_handle_t* out_handle)
{
    if (!out_handle) {
        return HAL_ERR_INVALID_PARAM;
    }
    const char* p = dev_path ? dev_path : "/dev/watchdog";

    struct hal_watchdog_handle_impl* h = (struct hal_watchdog_handle_impl*)calloc(1, sizeof(*h));
    if (!h) {
        return HAL_ERR_NO_MEMORY;
    }
    pthread_mutex_init(&h->mu, NULL);

#ifdef O_CLOEXEC
    h->fd = open(p, O_WRONLY | O_CLOEXEC);
#else
    h->fd = open(p, O_WRONLY);
#endif
    if (h->fd < 0) {
        pthread_mutex_destroy(&h->mu);
        free(h);
        return hal_status_from_errno(errno);
    }

    *out_handle = h;
    return HAL_OK;
}

hal_status_t hal_watchdog_close(hal_watchdog_handle_t handle)
{
    if (!handle) {
        return HAL_ERR_INVALID_PARAM;
    }
    struct hal_watchdog_handle_impl* h = (struct hal_watchdog_handle_impl*)handle;

    pthread_mutex_lock(&h->mu);
    int fd = h->fd;
    h->fd = -1;
    pthread_mutex_unlock(&h->mu);

    if (fd >= 0) {
        /* Magic close: disable watchdog on close if supported */
        int v = 'V';
        (void)write(fd, &v, 1);
        (void)close(fd);
    }
    pthread_mutex_destroy(&h->mu);
    free(h);
    return HAL_OK;
}

hal_status_t hal_watchdog_feed(hal_watchdog_handle_t handle)
{
    if (!handle) {
        return HAL_ERR_INVALID_PARAM;
    }
    struct hal_watchdog_handle_impl* h = (struct hal_watchdog_handle_impl*)handle;

    pthread_mutex_lock(&h->mu);
    int fd = h->fd;
    pthread_mutex_unlock(&h->mu);
    if (fd < 0) {
        return HAL_ERR_IO;
    }

    char z = 0;
    if (write(fd, &z, 1) != 1) {
        return hal_status_from_errno(errno);
    }
    return HAL_OK;
}

hal_status_t hal_watchdog_set_timeout(hal_watchdog_handle_t handle, unsigned seconds)
{
    if (!handle) {
        return HAL_ERR_INVALID_PARAM;
    }
    struct hal_watchdog_handle_impl* h = (struct hal_watchdog_handle_impl*)handle;

    pthread_mutex_lock(&h->mu);
    int fd = h->fd;
    pthread_mutex_unlock(&h->mu);
    if (fd < 0) {
        return HAL_ERR_IO;
    }

    int sec = (int)seconds;
    if (ioctl(fd, WDIOC_SETTIMEOUT, &sec) != 0) {
        return hal_status_from_errno(errno);
    }
    return HAL_OK;
}
