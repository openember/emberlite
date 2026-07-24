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

#include "ember/ppool_queue.h"
#include "ember/ppool_errno.h"

static int ppool_queue_get_insert_pos(pool_node *head, int priority)
{
    int pos;

    if (head == NULL) {
        return -1;
    }

    for (pos = 0; head && priority >= head->priority; ++pos) {
        head = head->next;
    }

    return pos;
}

pool_queue *ppool_queue_init(void)
{
    pool_queue *head;

    head = malloc(sizeof(pool_queue));
    if (!head) {
        ppool_errno = PE_QUEUE_NO_MEM;
        return NULL;
    }

    head->len = 0;
    head->head = NULL;

    return head;
}

pool_node *ppool_queue_new(void (*entry)(void *parameter), void *parameter, int priority)
{
    pool_node *node;

    if (priority < 0) {
        ppool_errno = PE_PRIORITY_ERROR;
        return NULL;
    }

    node = malloc(sizeof(pool_node));
    if (node == NULL) {
        ppool_errno = PE_QUEUE_NODE_NO_MEM;
        return NULL;
    }

    node->entry = entry;
    node->parameter = parameter;
    node->priority  = priority;
    node->next = NULL;

    return node;
}

void ppool_queue_add(pool_queue *queue, pool_node *node)
{
    int pos;
    int i;
    pool_node *h = queue->head;

    pos = ppool_queue_get_insert_pos(h, node->priority);

    if (pos == -1) {
        queue->head = node;
        queue->len += 1;
        return;
    }
    if (pos == 0) {
        node->next  = h;
        queue->head = node;
        queue->len += 1;
        return;
    }

    for (i = 0; i < pos - 1; ++i) {
        h = h->next;
    }

    node->next  = h->next;
    h->next     = node;
    queue->len += 1;
}

pool_node *ppool_queue_get_task(pool_queue *queue)
{
    pool_node *task;

    if (queue->head == NULL) {
        return NULL;
    }

    task = queue->head;
    queue->head = queue->head->next;
    queue->len -= 1;

    return task;
}

void ppool_queue_cleanup(pool_queue *queue)
{
    pool_node *h = queue->head;
    pool_node *temp;

    while (h) {
        temp = h;
        h = h->next;
        free(temp);
    }

    queue->len  = 0;
    queue->head = NULL;
}

void ppool_queue_destroy(pool_queue *queue)
{
    pool_node *h;
    pool_node *temp;

    if (!queue) {
        return;
    }

    h = queue->head;
    while (h) {
        temp = h;
        h = h->next;
        free(temp);
    }

    free(queue);
}
