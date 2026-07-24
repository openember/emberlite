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

#ifndef EMBER_PPOOL_QUEUE_H
#define EMBER_PPOOL_QUEUE_H

#include <stdlib.h>
#include <string.h>
#include <errno.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef struct ppool_node {
    void (*entry)(void *parameter);
    void *parameter;
    int   priority;
    struct ppool_node *next;
} pool_node;

typedef struct {
    int        len;
    pool_node *head;
} pool_queue;

pool_queue *ppool_queue_init(void);
pool_node  *ppool_queue_new(void (*entry)(void *parameter), void *parameter, int priority);
void        ppool_queue_add(pool_queue *queue, pool_node *node);
pool_node  *ppool_queue_get_task(pool_queue *queue);
void        ppool_queue_cleanup(pool_queue *queue);
void        ppool_queue_destroy(pool_queue *queue);

#ifdef __cplusplus
}
#endif

#endif /* EMBER_PPOOL_QUEUE_H */
