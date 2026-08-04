/*
 * Copyright (c) 2026, OpenEmber Team
 * SPDX-License-Identifier: Apache-2.0
 *
 * Lightweight table-driven finite state machine for EmberLite.
 */

#ifndef EMBER_FSM_H
#define EMBER_FSM_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint16_t ember_fsm_state_t;
typedef uint16_t ember_fsm_event_t;

typedef enum ember_fsm_result {
    EMBER_FSM_OK = 0,
    EMBER_FSM_EINVAL = -1,
    EMBER_FSM_ENOTFOUND = -2,
    EMBER_FSM_EACTION = -3,
} ember_fsm_result_t;

struct ember_fsm;

typedef int (*ember_fsm_action_t)(struct ember_fsm *fsm,
                                  ember_fsm_state_t from,
                                  ember_fsm_event_t event,
                                  ember_fsm_state_t to,
                                  void *user_data);

typedef struct ember_fsm_transition {
    ember_fsm_state_t from;
    ember_fsm_event_t event;
    ember_fsm_state_t to;
    ember_fsm_action_t action;
} ember_fsm_transition_t;

typedef struct ember_fsm {
    ember_fsm_state_t initial_state;
    ember_fsm_state_t current_state;
    const ember_fsm_transition_t *transitions;
    size_t transition_count;
    void *user_data;
} ember_fsm_t;

int ember_fsm_init(ember_fsm_t *fsm,
                   ember_fsm_state_t initial_state,
                   const ember_fsm_transition_t *transitions,
                   size_t transition_count,
                   void *user_data);

void ember_fsm_reset(ember_fsm_t *fsm);
ember_fsm_state_t ember_fsm_state(const ember_fsm_t *fsm);
void ember_fsm_set_user_data(ember_fsm_t *fsm, void *user_data);

int ember_fsm_dispatch(ember_fsm_t *fsm,
                       ember_fsm_event_t event,
                       ember_fsm_state_t *out_state);

#ifdef __cplusplus
}
#endif

#endif /* EMBER_FSM_H */
