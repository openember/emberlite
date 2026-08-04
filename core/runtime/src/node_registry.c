/*
 * Copyright (c) 2026, OpenEmber Team
 * SPDX-License-Identifier: Apache-2.0
 */

#include "ember/node_registry.h"

#include <string.h>

static size_t node_name_len(const char *name)
{
    size_t len = 0U;
    while (len < EMBER_NODE_NAME_MAX && name[len] != '\0') {
        len += 1U;
    }
    return len;
}

static int node_name_valid(const char *name)
{
    if (!name || name[0] == '\0') {
        return 0;
    }
    return node_name_len(name) < EMBER_NODE_NAME_MAX;
}

static void node_record_copy(ember_node_record_t *dst, const ember_node_record_t *src)
{
    memset(dst, 0, sizeof(*dst));
    if (src->name[0] != '\0') {
        strncpy(dst->name, src->name, sizeof(dst->name) - 1U);
    }
    dst->node_id = src->node_id;
    dst->kind = src->kind;
    dst->state = src->state;
    dst->last_heartbeat_ms = src->last_heartbeat_ms;
    dst->user_data = src->user_data;
}

int ember_node_registry_init(ember_node_registry_t *registry,
                             ember_node_record_t *storage,
                             size_t capacity)
{
    if (!registry || !storage || capacity == 0U) {
        return EMBER_NODE_REGISTRY_EINVAL;
    }

    registry->records = storage;
    registry->capacity = capacity;
    registry->count = 0U;
    memset(storage, 0, sizeof(storage[0]) * capacity);
    return EMBER_NODE_REGISTRY_OK;
}

void ember_node_registry_clear(ember_node_registry_t *registry)
{
    if (!registry || !registry->records) {
        return;
    }

    memset(registry->records, 0, sizeof(registry->records[0]) * registry->capacity);
    registry->count = 0U;
}

size_t ember_node_registry_count(const ember_node_registry_t *registry)
{
    return registry ? registry->count : 0U;
}

ember_node_record_t *ember_node_registry_find(ember_node_registry_t *registry,
                                              const char *name)
{
    if (!registry || !registry->records || !node_name_valid(name)) {
        return NULL;
    }

    for (size_t i = 0; i < registry->count; ++i) {
        if (strncmp(registry->records[i].name, name, EMBER_NODE_NAME_MAX) == 0) {
            return &registry->records[i];
        }
    }
    return NULL;
}

const ember_node_record_t *ember_node_registry_find_const(const ember_node_registry_t *registry,
                                                          const char *name)
{
    if (!registry || !registry->records || !node_name_valid(name)) {
        return NULL;
    }

    for (size_t i = 0; i < registry->count; ++i) {
        if (strncmp(registry->records[i].name, name, EMBER_NODE_NAME_MAX) == 0) {
            return &registry->records[i];
        }
    }
    return NULL;
}

ember_node_record_t *ember_node_registry_at(ember_node_registry_t *registry,
                                            size_t index)
{
    if (!registry || !registry->records || index >= registry->count) {
        return NULL;
    }
    return &registry->records[index];
}

const ember_node_record_t *ember_node_registry_at_const(const ember_node_registry_t *registry,
                                                        size_t index)
{
    if (!registry || !registry->records || index >= registry->count) {
        return NULL;
    }
    return &registry->records[index];
}

int ember_node_registry_register(ember_node_registry_t *registry,
                                 const ember_node_record_t *record)
{
    if (!registry || !registry->records || !record || !node_name_valid(record->name)) {
        return EMBER_NODE_REGISTRY_EINVAL;
    }
    if (registry->count >= registry->capacity) {
        return EMBER_NODE_REGISTRY_ENOSPC;
    }
    if (ember_node_registry_find(registry, record->name)) {
        return EMBER_NODE_REGISTRY_EDUP;
    }

    node_record_copy(&registry->records[registry->count], record);
    registry->count += 1U;
    return EMBER_NODE_REGISTRY_OK;
}

int ember_node_registry_unregister(ember_node_registry_t *registry,
                                   const char *name)
{
    if (!registry || !registry->records || !node_name_valid(name)) {
        return EMBER_NODE_REGISTRY_EINVAL;
    }

    for (size_t i = 0; i < registry->count; ++i) {
        if (strncmp(registry->records[i].name, name, EMBER_NODE_NAME_MAX) != 0) {
            continue;
        }

        for (size_t j = i + 1U; j < registry->count; ++j) {
            registry->records[j - 1U] = registry->records[j];
        }
        registry->count -= 1U;
        memset(&registry->records[registry->count], 0, sizeof(registry->records[0]));
        return EMBER_NODE_REGISTRY_OK;
    }

    return EMBER_NODE_REGISTRY_ENOTFOUND;
}

int ember_node_registry_update_state(ember_node_registry_t *registry,
                                     const char *name,
                                     ember_node_state_t state)
{
    ember_node_record_t *record = ember_node_registry_find(registry, name);
    if (!record) {
        return EMBER_NODE_REGISTRY_ENOTFOUND;
    }
    record->state = state;
    return EMBER_NODE_REGISTRY_OK;
}

int ember_node_registry_heartbeat(ember_node_registry_t *registry,
                                  const char *name,
                                  uint32_t now_ms)
{
    ember_node_record_t *record = ember_node_registry_find(registry, name);
    if (!record) {
        return EMBER_NODE_REGISTRY_ENOTFOUND;
    }
    record->last_heartbeat_ms = now_ms;
    return EMBER_NODE_REGISTRY_OK;
}
