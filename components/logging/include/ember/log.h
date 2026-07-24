/*
 * Copyright (c) 2026, OpenEmber Team
 * SPDX-License-Identifier: Apache-2.0
 *
 * EmberLite lightweight logging (builtin stderr).
 */

#ifndef EMBER_LOG_H
#define EMBER_LOG_H

#include <stdarg.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef enum ember_log_level {
    EMBER_LOG_LEVEL_DEBUG = 10,
    EMBER_LOG_LEVEL_INFO  = 20,
    EMBER_LOG_LEVEL_WARN  = 30,
    EMBER_LOG_LEVEL_ERROR = 40,
} ember_log_level_t;

/** Initialize process logging. Safe to call multiple times. */
int ember_log_init(const char *process_name);

/** Tear down logging. */
void ember_log_deinit(void);

void ember_log_vwrite(ember_log_level_t level, const char *tag, const char *fmt, va_list ap);
void ember_log_write(ember_log_level_t level, const char *tag, const char *fmt, ...);

#ifndef EMBER_LOG_TAG
#define EMBER_LOG_TAG ""
#endif

#define EMBER_LOGD(...) ember_log_write(EMBER_LOG_LEVEL_DEBUG, EMBER_LOG_TAG, __VA_ARGS__)
#define EMBER_LOGI(...) ember_log_write(EMBER_LOG_LEVEL_INFO, EMBER_LOG_TAG, __VA_ARGS__)
#define EMBER_LOGW(...) ember_log_write(EMBER_LOG_LEVEL_WARN, EMBER_LOG_TAG, __VA_ARGS__)
#define EMBER_LOGE(...) ember_log_write(EMBER_LOG_LEVEL_ERROR, EMBER_LOG_TAG, __VA_ARGS__)

#ifdef __cplusplus
}
#endif

#endif /* EMBER_LOG_H */
