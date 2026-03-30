/*
 * EmberLite HAL - I2S / PCM (ALSA)
 */

#include "hal/hal_i2s.h"

#include <errno.h>
#include <pthread.h>
#include <stdlib.h>
#include <string.h>

#if defined(HAL_HAS_ALSA) && HAL_HAS_ALSA
#include <alsa/asoundlib.h>
#include <poll.h>
#endif

struct hal_i2s_handle_impl {
#if defined(HAL_HAS_ALSA) && HAL_HAS_ALSA
    snd_pcm_t* pcm;
#endif
    hal_i2s_config_t cfg;
    size_t frame_bytes;
    pthread_mutex_t mu;
};

static size_t calc_frame_bytes(const hal_i2s_config_t* c)
{
    return (size_t)c->channels * ((size_t)c->bits_per_sample / 8u);
}

#if defined(HAL_HAS_ALSA) && HAL_HAS_ALSA

static snd_pcm_format_t bits_to_format(uint8_t bits)
{
    switch (bits) {
    case 16:
        return SND_PCM_FORMAT_S16_LE;
    case 24:
        return SND_PCM_FORMAT_S24_LE;
    case 32:
        return SND_PCM_FORMAT_S32_LE;
    default:
        return SND_PCM_FORMAT_UNKNOWN;
    }
}

static hal_status_t pcm_wait_ready(snd_pcm_t* pcm, int32_t timeout_ms)
{
    int tmo;
    if (timeout_ms < 0) {
        tmo = -1;
    } else {
        tmo = (int)timeout_ms;
    }
    int rc = snd_pcm_wait(pcm, tmo);
    if (rc == 0) {
        return HAL_ERR_TIMEOUT;
    }
    if (rc < 0) {
        return HAL_ERR_IO;
    }
    return HAL_OK;
}

hal_status_t hal_i2s_open(const hal_i2s_config_t* cfg, hal_i2s_handle_t* out_handle)
{
    if (!cfg || !out_handle || !cfg->device_name) {
        return HAL_ERR_INVALID_PARAM;
    }
    if (cfg->channels == 0 || cfg->sample_rate == 0) {
        return HAL_ERR_INVALID_PARAM;
    }
    if (cfg->bits_per_sample != 16 && cfg->bits_per_sample != 24 && cfg->bits_per_sample != 32) {
        return HAL_ERR_NOT_SUPPORTED;
    }

    snd_pcm_format_t fmt = bits_to_format(cfg->bits_per_sample);
    if (fmt == SND_PCM_FORMAT_UNKNOWN) {
        return HAL_ERR_NOT_SUPPORTED;
    }

    struct hal_i2s_handle_impl* h = (struct hal_i2s_handle_impl*)calloc(1, sizeof(*h));
    if (!h) {
        return HAL_ERR_NO_MEMORY;
    }
    h->cfg = *cfg;
    h->frame_bytes = calc_frame_bytes(cfg);
    pthread_mutex_init(&h->mu, NULL);

    snd_pcm_stream_t stream = cfg->capture ? SND_PCM_STREAM_CAPTURE : SND_PCM_STREAM_PLAYBACK;
    int err = snd_pcm_open(&h->pcm, cfg->device_name, stream, 0);
    if (err < 0) {
        pthread_mutex_destroy(&h->mu);
        free(h);
        return HAL_ERR_IO;
    }

    snd_pcm_hw_params_t* hw = NULL;
    snd_pcm_hw_params_alloca(&hw);
    snd_pcm_hw_params_any(h->pcm, hw);
    snd_pcm_hw_params_set_access(h->pcm, hw, SND_PCM_ACCESS_RW_INTERLEAVED);
    snd_pcm_hw_params_set_format(h->pcm, hw, fmt);
    snd_pcm_hw_params_set_channels(h->pcm, hw, cfg->channels);
    unsigned int rate = cfg->sample_rate;
    snd_pcm_hw_params_set_rate_near(h->pcm, hw, &rate, 0);
    err = snd_pcm_hw_params(h->pcm, hw);
    if (err < 0) {
        snd_pcm_close(h->pcm);
        pthread_mutex_destroy(&h->mu);
        free(h);
        return HAL_ERR_IO;
    }

    *out_handle = h;
    return HAL_OK;
}

hal_status_t hal_i2s_close(hal_i2s_handle_t handle)
{
    if (!handle) {
        return HAL_ERR_INVALID_PARAM;
    }
    struct hal_i2s_handle_impl* h = (struct hal_i2s_handle_impl*)handle;
    pthread_mutex_lock(&h->mu);
    if (h->pcm) {
        snd_pcm_close(h->pcm);
        h->pcm = NULL;
    }
    pthread_mutex_unlock(&h->mu);
    pthread_mutex_destroy(&h->mu);
    free(h);
    return HAL_OK;
}

hal_status_t hal_i2s_write(hal_i2s_handle_t handle, const uint8_t* buffer, size_t frames, int32_t timeout_ms)
{
    if (!handle || (!buffer && frames > 0)) {
        return HAL_ERR_INVALID_PARAM;
    }
    struct hal_i2s_handle_impl* h = (struct hal_i2s_handle_impl*)handle;
    if (frames == 0) {
        return HAL_OK;
    }
    if (h->cfg.capture) {
        return HAL_ERR_INVALID_PARAM;
    }

    pthread_mutex_lock(&h->mu);
    snd_pcm_t* pcm = h->pcm;
    pthread_mutex_unlock(&h->mu);
    if (!pcm) {
        return HAL_ERR_IO;
    }

    const uint8_t* p = buffer;
    snd_pcm_sframes_t left = (snd_pcm_sframes_t)frames;
    while (left > 0) {
        hal_status_t st = pcm_wait_ready(pcm, timeout_ms);
        if (st != HAL_OK) {
            return st;
        }
        snd_pcm_sframes_t w = snd_pcm_writei(pcm, p, (snd_pcm_uframes_t)left);
        if (w == -EPIPE) {
            (void)snd_pcm_prepare(pcm);
            continue;
        }
        if (w < 0) {
            return HAL_ERR_IO;
        }
        p += (size_t)w * h->frame_bytes;
        left -= w;
    }
    return HAL_OK;
}

hal_status_t hal_i2s_read(hal_i2s_handle_t handle, uint8_t* buffer, size_t frames, int32_t timeout_ms)
{
    if (!handle || (!buffer && frames > 0)) {
        return HAL_ERR_INVALID_PARAM;
    }
    struct hal_i2s_handle_impl* h = (struct hal_i2s_handle_impl*)handle;
    if (frames == 0) {
        return HAL_OK;
    }
    if (!h->cfg.capture) {
        return HAL_ERR_INVALID_PARAM;
    }

    pthread_mutex_lock(&h->mu);
    snd_pcm_t* pcm = h->pcm;
    pthread_mutex_unlock(&h->mu);
    if (!pcm) {
        return HAL_ERR_IO;
    }

    uint8_t* p = buffer;
    snd_pcm_sframes_t left = (snd_pcm_sframes_t)frames;
    while (left > 0) {
        hal_status_t st = pcm_wait_ready(pcm, timeout_ms);
        if (st != HAL_OK) {
            return st;
        }
        snd_pcm_sframes_t r = snd_pcm_readi(pcm, p, (snd_pcm_uframes_t)left);
        if (r == -EPIPE) {
            (void)snd_pcm_prepare(pcm);
            continue;
        }
        if (r < 0) {
            return HAL_ERR_IO;
        }
        p += (size_t)r * h->frame_bytes;
        left -= r;
    }
    return HAL_OK;
}

hal_status_t hal_i2s_start(hal_i2s_handle_t handle)
{
    if (!handle) {
        return HAL_ERR_INVALID_PARAM;
    }
    struct hal_i2s_handle_impl* h = (struct hal_i2s_handle_impl*)handle;
    pthread_mutex_lock(&h->mu);
    int err = h->pcm ? snd_pcm_start(h->pcm) : -1;
    pthread_mutex_unlock(&h->mu);
    return err == 0 ? HAL_OK : HAL_ERR_IO;
}

hal_status_t hal_i2s_stop(hal_i2s_handle_t handle)
{
    if (!handle) {
        return HAL_ERR_INVALID_PARAM;
    }
    struct hal_i2s_handle_impl* h = (struct hal_i2s_handle_impl*)handle;
    pthread_mutex_lock(&h->mu);
    int err = h->pcm ? snd_pcm_drop(h->pcm) : -1;
    pthread_mutex_unlock(&h->mu);
    return err == 0 ? HAL_OK : HAL_ERR_IO;
}

int hal_i2s_get_fd(hal_i2s_handle_t handle)
{
    if (!handle) {
        return -1;
    }
    struct hal_i2s_handle_impl* h = (struct hal_i2s_handle_impl*)handle;
    pthread_mutex_lock(&h->mu);
    int fd = -1;
    if (h->pcm) {
        int n = snd_pcm_poll_descriptors_count(h->pcm);
        if (n > 0) {
            struct pollfd pfd;
            memset(&pfd, 0, sizeof(pfd));
            if (snd_pcm_poll_descriptors(h->pcm, &pfd, 1) == 1) {
                fd = pfd.fd;
            }
        }
    }
    pthread_mutex_unlock(&h->mu);
    return fd;
}

#else /* !HAL_HAS_ALSA */

hal_status_t hal_i2s_open(const hal_i2s_config_t* cfg, hal_i2s_handle_t* out_handle)
{
    (void)cfg;
    (void)out_handle;
    return HAL_ERR_NOT_SUPPORTED;
}

hal_status_t hal_i2s_close(hal_i2s_handle_t handle)
{
    (void)handle;
    return HAL_ERR_INVALID_PARAM;
}

hal_status_t hal_i2s_write(hal_i2s_handle_t handle,
                           const uint8_t* buffer,
                           size_t frames,
                           int32_t timeout_ms)
{
    (void)handle;
    (void)buffer;
    (void)frames;
    (void)timeout_ms;
    return HAL_ERR_NOT_SUPPORTED;
}

hal_status_t hal_i2s_read(hal_i2s_handle_t handle,
                          uint8_t* buffer,
                          size_t frames,
                          int32_t timeout_ms)
{
    (void)handle;
    (void)buffer;
    (void)frames;
    (void)timeout_ms;
    return HAL_ERR_NOT_SUPPORTED;
}

hal_status_t hal_i2s_start(hal_i2s_handle_t handle)
{
    (void)handle;
    return HAL_ERR_NOT_SUPPORTED;
}

hal_status_t hal_i2s_stop(hal_i2s_handle_t handle)
{
    (void)handle;
    return HAL_ERR_NOT_SUPPORTED;
}

int hal_i2s_get_fd(hal_i2s_handle_t handle)
{
    (void)handle;
    return -1;
}

#endif
