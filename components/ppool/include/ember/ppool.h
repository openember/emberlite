/*
 * Copyright (c) 2022-2023, OpenEmber Team
 *
 * SPDX-License-Identifier: Apache-2.0
 *
 * Change Logs:
 * Date           Author       Notes
 * 2015-06-30     briskgreen   the first version
 * 2022-11-02     luhuadong    optimize code style
 * 2026-07-25     openember    migrate to emberlite (standalone C)
 */

#ifndef EMBER_PPOOL_H
#define EMBER_PPOOL_H

#include <pthread.h>
#include <stdbool.h>

#include "ember/ppool_queue.h"

#ifdef __cplusplus
extern "C" {
#endif

/* 全局互斥锁：只声明，不在头文件里定义，避免多翻译单元重复定义 */
extern pthread_mutex_t PPOOL_LOCK;

#define ppool_entry() pthread_mutex_lock(&PPOOL_LOCK)
#define ppool_leave() pthread_mutex_unlock(&PPOOL_LOCK)

typedef struct {
    int pool_max_num;  /* 线程池最大线程数量 */
    int rel_num;       /* 线程池中实例线程数 */
    pool_queue *queue; /* 任务队列头指针 */
    pthread_t  *id;    /* 线程 ID 号 */

    pthread_mutex_t ppool_lock;
    pthread_cond_t  ppool_cond;
} pool_t;

typedef struct {
    void (*entry)(void *parameter); /* 任务 */
    void  *parameter;               /* 参数 */
    int    priority;                /* 优先级 */
} pool_task;

pool_t *ppool_init(int pool_max_num);
bool    ppool_add(pool_t *pool, pool_task *task);
void    ppool_destroy(pool_t *pool);

#ifdef __cplusplus
}
#endif

#endif /* EMBER_PPOOL_H */
