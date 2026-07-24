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

#include <sys/prctl.h>

#include "ember/ppool.h"
#include "ember/ppool_errno.h"

pthread_mutex_t PPOOL_LOCK;

static void *ppool_run(void *arg)
{
    pool_t *pool = (pool_t *)arg;
    pool_node *task;

    prctl(PR_SET_NAME, "threadpool");

    while (1) {
        pthread_mutex_lock(&pool->ppool_lock);

        while (pool->queue->len <= 0) {
            pthread_cond_wait(&pool->ppool_cond, &pool->ppool_lock);
        }
        task = ppool_queue_get_task(pool->queue);

        pthread_mutex_unlock(&pool->ppool_lock);

        if (task == NULL) {
            continue;
        }
        task->entry(task->parameter);

        free(task);
    }

    return NULL;
}

pool_t *ppool_init(int pool_max_num)
{
    pool_t     *pool;
    pool_queue *queue;
    int i;

    if (pool_max_num <= 0) {
        ppool_errno = PE_PRIORITY_ERROR;
        return NULL;
    }

    pool = malloc(sizeof(pool_t));
    if (!pool) {
        ppool_errno = PE_POOL_NO_MEM;
        return NULL;
    }

    queue = ppool_queue_init();
    if (!queue) {
        free(pool);
        return NULL;
    }

    pool->pool_max_num = pool_max_num;
    pool->rel_num = 0;
    pool->queue = queue;
    pool->id = malloc(sizeof(pthread_t) * (size_t)pool_max_num);
    if (!pool->id) {
        ppool_errno = PE_THREAD_NO_MEM;
        free(queue);
        free(pool);
        return NULL;
    }

    if (pthread_mutex_init(&pool->ppool_lock, NULL) != 0) {
        ppool_errno = PE_THREAD_MUTEX_ERROR;
        free(pool->id);
        free(queue);
        free(pool);
        return NULL;
    }

    if (pthread_mutex_init(&PPOOL_LOCK, NULL) != 0) {
        ppool_errno = PE_THREAD_MUTEX_ERROR;
        free(pool->id);
        free(queue);
        pthread_mutex_destroy(&pool->ppool_lock);
        free(pool);
        return NULL;
    }

    if (pthread_cond_init(&pool->ppool_cond, NULL) != 0) {
        ppool_errno = PE_THREAD_COND_ERROR;
        free(pool->id);
        free(queue);
        pthread_mutex_destroy(&pool->ppool_lock);
        pthread_mutex_destroy(&PPOOL_LOCK);
        free(pool);
        return NULL;
    }

    for (i = 0; i < pool_max_num; ++i) {
        if (pthread_create(&pool->id[i], NULL, ppool_run, pool) == 0) {
            ++pool->rel_num;
            pthread_detach(pool->id[i]);
        }
    }

    return pool;
}

bool ppool_add(pool_t *pool, pool_task *task)
{
    pool_node *node;

    if (!pool || !task || !task->entry) {
        return false;
    }

    node = ppool_queue_new(task->entry, task->parameter, task->priority);
    if (!node) {
        return false;
    }

    pthread_mutex_lock(&pool->ppool_lock);
    ppool_queue_add(pool->queue, node);
    pthread_cond_broadcast(&pool->ppool_cond);
    pthread_mutex_unlock(&pool->ppool_lock);

    return true;
}

void ppool_destroy(pool_t *pool)
{
    int i;

    if (!pool) {
        return;
    }

    ppool_queue_destroy(pool->queue);

    for (i = 0; i < pool->pool_max_num; ++i) {
        if (pool->id[i]) {
            pthread_cancel(pool->id[i]);
        }
    }
    free(pool->id);

    pthread_mutex_destroy(&pool->ppool_lock);
    pthread_mutex_destroy(&PPOOL_LOCK);
    pthread_cond_destroy(&pool->ppool_cond);

    free(pool);
}
