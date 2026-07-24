/*
 * Copyright (c) 2026, OpenEmber Team
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ember/log.h"

#include <stdio.h>
#include <string.h>

static int g_inited = 0;
static char g_proc[64] = "emberlite";

int ember_log_init(const char *process_name)
{
    if (process_name && process_name[0]) {
        snprintf(g_proc, sizeof(g_proc), "%s", process_name);
    }
    g_inited = 1;
    return 0;
}

void ember_log_deinit(void)
{
    g_inited = 0;
}

static const char *level_prefix(ember_log_level_t level)
{
    switch (level) {
    case EMBER_LOG_LEVEL_DEBUG:
        return "D";
    case EMBER_LOG_LEVEL_INFO:
        return "I";
    case EMBER_LOG_LEVEL_WARN:
        return "W";
    case EMBER_LOG_LEVEL_ERROR:
    default:
        return "E";
    }
}

void ember_log_vwrite(ember_log_level_t level, const char *tag, const char *fmt, va_list ap)
{
    const char *t = (tag && tag[0]) ? tag : "";
    fprintf(stderr, "[%s] [%s]", level_prefix(level), g_proc);
    if (t[0]) {
        fprintf(stderr, " [%s]", t);
    }
    fprintf(stderr, " ");
    vfprintf(stderr, fmt ? fmt : "", ap);
    fprintf(stderr, "\n");
    (void)g_inited;
}

void ember_log_write(ember_log_level_t level, const char *tag, const char *fmt, ...)
{
    va_list ap;
    va_start(ap, fmt);
    ember_log_vwrite(level, tag, fmt, ap);
    va_end(ap);
}
