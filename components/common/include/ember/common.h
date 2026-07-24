/*
 * Copyright (c) 2022-2026, OpenEmber Team
 * SPDX-License-Identifier: Apache-2.0
 *
 * EmberLite common utilities (migrated from OpenEmber components/Common).
 */

#ifndef EMBER_COMMON_H
#define EMBER_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif

/** Print a greeting line to stdout. */
void ember_say_hello(const char *name);

/** Pseudo-random int (seeded once from time/pid). */
int ember_random(void);

/** Seconds since Unix epoch (or 0 on failure). */
int ember_time(void);


#ifdef __cplusplus
}
#endif

#endif /* EMBER_COMMON_H */
