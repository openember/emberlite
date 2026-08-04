/*
 * Copyright (c) 2026, OpenEmber Team
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ember/fsm.h"

int ember_fsm_init(ember_fsm_t *fsm,
                   ember_fsm_state_t initial_state,
                   const ember_fsm_transition_t *transitions,
                   size_t transition_count,
                   void *user_data)
{
    if (!fsm || (!transitions && transition_count > 0U)) {
        return EMBER_FSM_EINVAL;
    }

    fsm->initial_state = initial_state;
    fsm->current_state = initial_state;
    fsm->transitions = transitions;
    fsm->transition_count = transition_count;
    fsm->user_data = user_data;
    return EMBER_FSM_OK;
}

void ember_fsm_reset(ember_fsm_t *fsm)
{
    if (!fsm) {
        return;
    }
    fsm->current_state = fsm->initial_state;
}

ember_fsm_state_t ember_fsm_state(const ember_fsm_t *fsm)
{
    return fsm ? fsm->current_state : 0U;
}

void ember_fsm_set_user_data(ember_fsm_t *fsm, void *user_data)
{
    if (!fsm) {
        return;
    }
    fsm->user_data = user_data;
}

int ember_fsm_dispatch(ember_fsm_t *fsm,
                       ember_fsm_event_t event,
                       ember_fsm_state_t *out_state)
{
    if (!fsm) {
        return EMBER_FSM_EINVAL;
    }

    for (size_t i = 0; i < fsm->transition_count; ++i) {
        const ember_fsm_transition_t *transition = &fsm->transitions[i];
        if (transition->from != fsm->current_state || transition->event != event) {
            continue;
        }

        const ember_fsm_state_t from = fsm->current_state;
        if (transition->action) {
            const int action_result =
                transition->action(fsm, from, event, transition->to, fsm->user_data);
            if (action_result != 0) {
                return EMBER_FSM_EACTION;
            }
        }

        fsm->current_state = transition->to;
        if (out_state) {
            *out_state = fsm->current_state;
        }
        return EMBER_FSM_OK;
    }

    if (out_state) {
        *out_state = fsm->current_state;
    }
    return EMBER_FSM_ENOTFOUND;
}
