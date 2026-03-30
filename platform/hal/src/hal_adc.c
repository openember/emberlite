/*
 * EmberLite HAL - ADC (Linux IIO sysfs)
 */

#include "hal/hal_adc.h"

#include <errno.h>
#include <fcntl.h>
#include <pthread.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

struct hal_adc_handle_impl {
    char base_path[256];
    uint32_t channel_index;
    float scale_to_mv; /* raw * scale_to_mv -> millivolts */
    pthread_mutex_t mu;
};

static int read_file_trimmed(const char* path, char* buf, size_t buf_len)
{
    int fd = open(path, O_RDONLY | O_CLOEXEC);
    if (fd < 0) {
        return -1;
    }
    ssize_t r = read(fd, buf, buf_len - 1);
    (void)close(fd);
    if (r < 0) {
        return -1;
    }
    buf[r] = '\0';
    /* trim */
    for (ssize_t i = r - 1; i >= 0; i--) {
        if (buf[i] == '\n' || buf[i] == '\r') {
            buf[i] = '\0';
        } else {
            break;
        }
    }
    return 0;
}

static float parse_scale_to_mv(const char* s)
{
    /* IIO scale is often in V (e.g. "0.805664062") */
    char* end = NULL;
    errno = 0;
    float v = strtof(s, &end);
    if (errno != 0 || !end || *end != '\0') {
        return 0.0f;
    }
    return v * 1000.0f;
}

hal_status_t hal_adc_open(const hal_adc_config_t* config, hal_adc_handle_t* out_handle)
{
    if (!config || !out_handle || !config->iio_path) {
        return HAL_ERR_INVALID_PARAM;
    }

    struct hal_adc_handle_impl* h = (struct hal_adc_handle_impl*)calloc(1, sizeof(*h));
    if (!h) {
        return HAL_ERR_NO_MEMORY;
    }

    snprintf(h->base_path, sizeof(h->base_path), "%s", config->iio_path);
    h->channel_index = config->channel_index;
    h->scale_to_mv = 1.0f;
    pthread_mutex_init(&h->mu, NULL);

    char scale_path[320];
    snprintf(scale_path, sizeof(scale_path), "%s/in_voltage%u_scale", h->base_path, h->channel_index);
    char buf[128];
    if (read_file_trimmed(scale_path, buf, sizeof(buf)) == 0) {
        h->scale_to_mv = parse_scale_to_mv(buf);
    }

    *out_handle = h;
    return HAL_OK;
}

hal_status_t hal_adc_close(hal_adc_handle_t handle)
{
    if (!handle) {
        return HAL_ERR_INVALID_PARAM;
    }
    struct hal_adc_handle_impl* h = (struct hal_adc_handle_impl*)handle;
    pthread_mutex_destroy(&h->mu);
    free(h);
    return HAL_OK;
}

hal_status_t hal_adc_read_raw(hal_adc_handle_t handle, uint32_t* out_value)
{
    if (!handle || !out_value) {
        return HAL_ERR_INVALID_PARAM;
    }
    struct hal_adc_handle_impl* h = (struct hal_adc_handle_impl*)handle;

    char path[320];
    char buf[64];
    snprintf(path, sizeof(path), "%s/in_voltage%u_raw", h->base_path, h->channel_index);

    pthread_mutex_lock(&h->mu);
    int rc = read_file_trimmed(path, buf, sizeof(buf));
    pthread_mutex_unlock(&h->mu);
    if (rc != 0) {
        return hal_status_from_errno(errno);
    }

    errno = 0;
    char* end = NULL;
    long v = strtol(buf, &end, 10);
    if (errno != 0 || !end || *end != '\0' || v < 0) {
        return HAL_ERR_IO;
    }
    *out_value = (uint32_t)v;
    return HAL_OK;
}

hal_status_t hal_adc_read_voltage(hal_adc_handle_t handle, uint32_t* out_mv)
{
    if (!handle || !out_mv) {
        return HAL_ERR_INVALID_PARAM;
    }

    uint32_t raw = 0;
    hal_status_t st = hal_adc_read_raw(handle, &raw);
    if (st != HAL_OK) {
        return st;
    }

    struct hal_adc_handle_impl* h = (struct hal_adc_handle_impl*)handle;
    float mv = (float)raw * h->scale_to_mv;
    if (mv < 0.0f) {
        mv = 0.0f;
    }
    *out_mv = (uint32_t)(mv + 0.5f);
    return HAL_OK;
}
