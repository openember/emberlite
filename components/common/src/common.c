/*
 * Copyright (c) 2022-2026, OpenEmber Team
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ember/common.h"

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <unistd.h>

void ember_say_hello(const char *name)
{
    assert(name);
    printf("Hello, %s!\n", name);
}

int ember_random(void)
{
    static int seeded = 0;
    if (!seeded) {
        unsigned seed = (unsigned)time(NULL) ^ ((unsigned)getpid() << 16);
        srand(seed);
        seeded = 1;
    }
    return rand();
}

int ember_time(void)
{
    return (int)time(NULL);
}
