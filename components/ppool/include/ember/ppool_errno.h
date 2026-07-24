/*
 * Copyright (c) 2022-2023, OpenEmber Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2015-06-30     briskgreen   the first version
 * 2022-11-02     luhuadong    optimize code style
 * 2026-07-25     openember    migrate to emberlite
 */

#ifndef EMBER_PPOOL_ERRNO_H
#define EMBER_PPOOL_ERRNO_H

#include <stdio.h>
#include <string.h>

#ifdef __cplusplus
extern "C" {
#endif

extern int ppool_errno;

#define PE_OK                  0
#define PE_POOL_NO_MEM        -1
#define PE_THREAD_NO_MEM      -2
#define PE_THREAD_MUTEX_ERROR -3
#define PE_THREAD_COND_ERROR  -4
#define PE_QUEUE_NO_MEM       -5
#define PE_PRIORITY_ERROR     -6
#define PE_QUEUE_NODE_NO_MEM  -7

void ppool_error(const char *msg);
char *ppool_strerr(int err);

#ifdef __cplusplus
}
#endif

#endif /* EMBER_PPOOL_ERRNO_H */
