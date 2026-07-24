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

#include "ember/ppool_errno.h"

int ppool_errno = 0;

void ppool_error(const char *msg)
{
    if (!msg) {
        printf("%s\n", ppool_strerr(ppool_errno));
    } else {
        printf("%s : %s\n", ppool_strerr(ppool_errno), msg);
    }
}

char *ppool_strerr(int err)
{
    switch (err) {
    case 0:
        return "成功!";
    case -1:
        return "无法为线程池开辟空间，创建线程池失败!";
    case -2:
        return "无法为此数量的线程分配足够的内存!";
    case -3:
        return "pthread初始化互斥锁失败，请使用ppool_error查看更多信息!";
    case -4:
        return "pthread初始化条件变量失败，请使用ppool_error查看更多信息!";
    case -5:
        return "无法为任务队列开辟空间!";
    case -6:
        return "错误的优先级!";
    case -7:
        return "无法为队列创建一个结点，开辟内存出错!";
    default:
        return "未知错误!";
    }
}
