/*
 * Copyright (c) 2026, OpenEmber Team
 * SPDX-License-Identifier: Apache-2.0
 *
 * Fixed-capacity node registry for EmberLite.
 */

#ifndef EMBER_NODE_REGISTRY_H
#define EMBER_NODE_REGISTRY_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#ifndef EMBER_NODE_NAME_MAX
#define EMBER_NODE_NAME_MAX 32
#endif

typedef enum ember_node_registry_result {
    EMBER_NODE_REGISTRY_OK = 0,
    EMBER_NODE_REGISTRY_EINVAL = -1,
    EMBER_NODE_REGISTRY_ENOSPC = -2,
    EMBER_NODE_REGISTRY_ENOTFOUND = -3,
    EMBER_NODE_REGISTRY_EDUP = -4,
} ember_node_registry_result_t;

typedef enum ember_node_kind {
    EMBER_NODE_KIND_UNKNOWN = 0,
    EMBER_NODE_KIND_SYSTEM,
    EMBER_NODE_KIND_SERVICE,
    EMBER_NODE_KIND_APPLICATION,
    EMBER_NODE_KIND_DEVICE,
    EMBER_NODE_KIND_DRIVER,
    EMBER_NODE_KIND_TOOL,
} ember_node_kind_t;

typedef enum ember_node_state {
    EMBER_NODE_STATE_UNKNOWN = 0,
    EMBER_NODE_STATE_INIT,
    EMBER_NODE_STATE_READY,
    EMBER_NODE_STATE_RUNNING,
    EMBER_NODE_STATE_DEGRADED,
    EMBER_NODE_STATE_ERROR,
    EMBER_NODE_STATE_STOPPED,
} ember_node_state_t;

typedef struct ember_node_record {
    char name[EMBER_NODE_NAME_MAX];
    uint16_t node_id;
    ember_node_kind_t kind;
    ember_node_state_t state;
    uint32_t last_heartbeat_ms;
    void *user_data;
} ember_node_record_t;

typedef struct ember_node_registry {
    ember_node_record_t *records;
    size_t capacity;
    size_t count;
} ember_node_registry_t;

int ember_node_registry_init(ember_node_registry_t *registry,
                             ember_node_record_t *storage,
                             size_t capacity);

void ember_node_registry_clear(ember_node_registry_t *registry);
size_t ember_node_registry_count(const ember_node_registry_t *registry);

int ember_node_registry_register(ember_node_registry_t *registry,
                                 const ember_node_record_t *record);

int ember_node_registry_unregister(ember_node_registry_t *registry,
                                   const char *name);

ember_node_record_t *ember_node_registry_find(ember_node_registry_t *registry,
                                              const char *name);

const ember_node_record_t *ember_node_registry_find_const(const ember_node_registry_t *registry,
                                                          const char *name);

ember_node_record_t *ember_node_registry_at(ember_node_registry_t *registry,
                                            size_t index);

const ember_node_record_t *ember_node_registry_at_const(const ember_node_registry_t *registry,
                                                        size_t index);

int ember_node_registry_update_state(ember_node_registry_t *registry,
                                     const char *name,
                                     ember_node_state_t state);

int ember_node_registry_heartbeat(ember_node_registry_t *registry,
                                  const char *name,
                                  uint32_t now_ms);

#ifdef __cplusplus
}
#endif

#endif /* EMBER_NODE_REGISTRY_H */
