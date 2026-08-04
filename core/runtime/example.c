/*
 * Copyright (c) 2026, OpenEmber Team
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ember/runtime.h"

#include <stdio.h>

enum {
    DEMO_STATE_BOOT = 1,
    DEMO_STATE_READY,
    DEMO_STATE_RUNNING,
    DEMO_STATE_ERROR,
};

enum {
    DEMO_EVENT_READY = 1,
    DEMO_EVENT_START,
    DEMO_EVENT_FAULT,
    DEMO_EVENT_RESET,
};

static int on_transition(ember_fsm_t *fsm,
                         ember_fsm_state_t from,
                         ember_fsm_event_t event,
                         ember_fsm_state_t to,
                         void *user_data)
{
    (void)fsm;
    (void)user_data;
    printf("fsm: %u --%u--> %u\n", (unsigned)from, (unsigned)event, (unsigned)to);
    return 0;
}

int main(void)
{
    static const ember_fsm_transition_t transitions[] = {
        {DEMO_STATE_BOOT, DEMO_EVENT_READY, DEMO_STATE_READY, on_transition},
        {DEMO_STATE_READY, DEMO_EVENT_START, DEMO_STATE_RUNNING, on_transition},
        {DEMO_STATE_RUNNING, DEMO_EVENT_FAULT, DEMO_STATE_ERROR, on_transition},
        {DEMO_STATE_ERROR, DEMO_EVENT_RESET, DEMO_STATE_READY, on_transition},
    };

    ember_fsm_t fsm;
    if (ember_fsm_init(&fsm, DEMO_STATE_BOOT, transitions,
                       sizeof(transitions) / sizeof(transitions[0]), NULL) != EMBER_FSM_OK) {
        return 1;
    }

    (void)ember_fsm_dispatch(&fsm, DEMO_EVENT_READY, NULL);
    (void)ember_fsm_dispatch(&fsm, DEMO_EVENT_START, NULL);
    (void)ember_fsm_dispatch(&fsm, DEMO_EVENT_FAULT, NULL);

    ember_node_record_t storage[4];
    ember_node_registry_t registry;
    if (ember_node_registry_init(&registry, storage, sizeof(storage) / sizeof(storage[0]))
        != EMBER_NODE_REGISTRY_OK) {
        return 1;
    }

    const ember_node_record_t app = {
        .name = "product_app",
        .node_id = 1,
        .kind = EMBER_NODE_KIND_APPLICATION,
        .state = EMBER_NODE_STATE_RUNNING,
        .last_heartbeat_ms = 1000,
        .user_data = NULL,
    };

    if (ember_node_registry_register(&registry, &app) != EMBER_NODE_REGISTRY_OK) {
        return 1;
    }
    (void)ember_node_registry_heartbeat(&registry, "product_app", 1200);

    const ember_node_record_t *record = ember_node_registry_find_const(&registry, "product_app");
    if (!record) {
        return 1;
    }

    printf("node: %s state=%d heartbeat=%u\n",
           record->name,
           (int)record->state,
           (unsigned)record->last_heartbeat_ms);
    return 0;
}
