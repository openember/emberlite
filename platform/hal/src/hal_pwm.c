/*
 * EmberLite HAL - PWM (Linux sysfs pwmchip)
 */

#include "hal/hal_pwm.h"

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct hal_pwm_handle_impl {
    char chip_path[256];
    uint32_t channel;
    char pwm_dir[320];
    uint32_t period_ns;
    uint8_t enabled;
    pthread_mutex_t mu;
};

static int write_sysfs(const char* path, const char* s)
{
    int fd = open(path, O_WRONLY | O_CLOEXEC);
    if (fd < 0) {
        return -1;
    }
    size_t len = strlen(s);
    ssize_t w = write(fd, s, len);
    int saved = errno;
    (void)close(fd);
    errno = saved;
    return (w == (ssize_t)len) ? 0 : -1;
}

static hal_status_t pwm_export(const char* chip_path, uint32_t channel)
{
    char path[384];
    char buf[32];
    snprintf(path, sizeof(path), "%s/export", chip_path);
    snprintf(buf, sizeof(buf), "%u", channel);
    if (write_sysfs(path, buf) != 0) {
        if (errno == EBUSY) {
            return HAL_OK;
        }
        return hal_status_from_errno(errno);
    }
    return HAL_OK;
}

static void pwm_try_unexport(const char* chip_path, uint32_t channel)
{
    char path[384];
    char buf[32];
    snprintf(path, sizeof(path), "%s/unexport", chip_path);
    snprintf(buf, sizeof(buf), "%u", channel);
    (void)write_sysfs(path, buf);
}

hal_status_t hal_pwm_open(const hal_pwm_config_t* config, hal_pwm_handle_t* out_handle)
{
    if (!config || !out_handle || !config->chip_path) {
        return HAL_ERR_INVALID_PARAM;
    }

    hal_status_t st = pwm_export(config->chip_path, config->channel);
    if (st != HAL_OK) {
        return st;
    }

    struct hal_pwm_handle_impl* h = (struct hal_pwm_handle_impl*)calloc(1, sizeof(*h));
    if (!h) {
        return HAL_ERR_NO_MEMORY;
    }

    snprintf(h->chip_path, sizeof(h->chip_path), "%s", config->chip_path);
    h->channel = config->channel;
    snprintf(h->pwm_dir, sizeof(h->pwm_dir), "%s/pwm%u", config->chip_path, config->channel);
    h->period_ns = 1000000u;
    h->enabled = 0;
    pthread_mutex_init(&h->mu, NULL);

    *out_handle = h;
    return HAL_OK;
}

hal_status_t hal_pwm_close(hal_pwm_handle_t handle)
{
    if (!handle) {
        return HAL_ERR_INVALID_PARAM;
    }
    struct hal_pwm_handle_impl* h = (struct hal_pwm_handle_impl*)handle;

    pthread_mutex_lock(&h->mu);
    char chip[256];
    uint32_t ch = h->channel;
    snprintf(chip, sizeof(chip), "%s", h->chip_path);
    pthread_mutex_unlock(&h->mu);

    pwm_try_unexport(chip, ch);

    pthread_mutex_destroy(&h->mu);
    free(h);
    return HAL_OK;
}

hal_status_t hal_pwm_set_frequency(hal_pwm_handle_t handle, uint32_t hz)
{
    if (!handle || hz == 0) {
        return HAL_ERR_INVALID_PARAM;
    }
    struct hal_pwm_handle_impl* h = (struct hal_pwm_handle_impl*)handle;

    uint64_t period = 1000000000ULL / (uint64_t)hz;
    if (period == 0 || period > 0xFFFFFFFFu) {
        return HAL_ERR_INVALID_PARAM;
    }

    pthread_mutex_lock(&h->mu);
    h->period_ns = (uint32_t)period;
    char path[400];
    char buf[32];
    snprintf(path, sizeof(path), "%s/period", h->pwm_dir);
    snprintf(buf, sizeof(buf), "%u", h->period_ns);
    int rc = write_sysfs(path, buf);
    pthread_mutex_unlock(&h->mu);
    if (rc != 0) {
        return hal_status_from_errno(errno);
    }
    return HAL_OK;
}

hal_status_t hal_pwm_set_duty_cycle(hal_pwm_handle_t handle, float percent)
{
    if (!handle) {
        return HAL_ERR_INVALID_PARAM;
    }
    if (percent < 0.0f || percent > 100.0f) {
        return HAL_ERR_INVALID_PARAM;
    }
    struct hal_pwm_handle_impl* h = (struct hal_pwm_handle_impl*)handle;

    unsigned long pct = (unsigned long)(percent * 10000.0f + 0.5f); /* 0..1000000 = 0..100.0000% */
    if (pct > 1000000UL) {
        pct = 1000000UL;
    }
    uint32_t duty_ns = (uint32_t)(((uint64_t)h->period_ns * pct) / 1000000ULL);
    if (duty_ns > h->period_ns) {
        duty_ns = h->period_ns;
    }

    pthread_mutex_lock(&h->mu);
    char path[400];
    char buf[32];
    snprintf(path, sizeof(path), "%s/duty_cycle", h->pwm_dir);
    snprintf(buf, sizeof(buf), "%u", duty_ns);
    int rc = write_sysfs(path, buf);
    pthread_mutex_unlock(&h->mu);
    if (rc != 0) {
        return hal_status_from_errno(errno);
    }
    return HAL_OK;
}

hal_status_t hal_pwm_enable(hal_pwm_handle_t handle)
{
    if (!handle) {
        return HAL_ERR_INVALID_PARAM;
    }
    struct hal_pwm_handle_impl* h = (struct hal_pwm_handle_impl*)handle;

    pthread_mutex_lock(&h->mu);
    char path[400];
    snprintf(path, sizeof(path), "%s/enable", h->pwm_dir);
    int rc = write_sysfs(path, "1");
    if (rc == 0) {
        h->enabled = 1;
    }
    pthread_mutex_unlock(&h->mu);
    if (rc != 0) {
        return hal_status_from_errno(errno);
    }
    return HAL_OK;
}

hal_status_t hal_pwm_disable(hal_pwm_handle_t handle)
{
    if (!handle) {
        return HAL_ERR_INVALID_PARAM;
    }
    struct hal_pwm_handle_impl* h = (struct hal_pwm_handle_impl*)handle;

    pthread_mutex_lock(&h->mu);
    char path[400];
    snprintf(path, sizeof(path), "%s/enable", h->pwm_dir);
    int rc = write_sysfs(path, "0");
    if (rc == 0) {
        h->enabled = 0;
    }
    pthread_mutex_unlock(&h->mu);
    if (rc != 0) {
        return hal_status_from_errno(errno);
    }
    return HAL_OK;
}

int hal_pwm_get_fd(hal_pwm_handle_t handle)
{
    (void)handle;
    return -1;
}
