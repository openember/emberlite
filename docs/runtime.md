# Runtime 组件

`core/runtime` 提供 EmberLite 中最基础的运行时辅助能力，构建产物为 `ember_runtime`。第一版包含两个小模块：

- `ember/fsm.h`：轻量表驱动状态机。
- `ember/node_registry.h`：固定容量节点注册表。

设计原则：

- 位于 `core/runtime`，属于 EmberLite Core 层，而不是普通 Components。
- 纯 C11 API，适合嵌入式 Linux、RTOS 或裸机移植。
- 不在组件内部申请内存；所有存储由调用者提供。
- 不绑定 Linux PID、线程、进程监督或消息中间件。
- 与 OpenEmber 的新框架语义保持靠近，但不迁移旧 SMM/FSM 实现。

## FSM

FSM 使用状态、事件、迁移表和可选 action 回调描述行为：

```c
#include "ember/fsm.h"

enum {
    STATE_BOOT = 1,
    STATE_READY,
    STATE_RUNNING,
};

enum {
    EVENT_READY = 1,
    EVENT_START,
};

static const ember_fsm_transition_t transitions[] = {
    {STATE_BOOT, EVENT_READY, STATE_READY, NULL},
    {STATE_READY, EVENT_START, STATE_RUNNING, NULL},
};

ember_fsm_t fsm;
ember_fsm_init(&fsm, STATE_BOOT, transitions,
               sizeof(transitions) / sizeof(transitions[0]), NULL);
ember_fsm_dispatch(&fsm, EVENT_READY, NULL);
```

如果没有匹配迁移，`ember_fsm_dispatch()` 返回 `EMBER_FSM_ENOTFOUND`，当前状态保持不变。

## Node Registry

节点注册表用于小系统中维护已知节点、节点类型、当前状态和最近心跳时间。它类似旧 SMM 的“注册表”概念，但不包含 PID、kill、动态链表或 pthread 锁。

```c
#include "ember/node_registry.h"

ember_node_record_t storage[8];
ember_node_registry_t registry;
ember_node_registry_init(&registry, storage, 8);

ember_node_record_t app = {
    .name = "product_app",
    .node_id = 1,
    .kind = EMBER_NODE_KIND_APPLICATION,
    .state = EMBER_NODE_STATE_RUNNING,
};

ember_node_registry_register(&registry, &app);
ember_node_registry_heartbeat(&registry, "product_app", now_ms);
```

后续如果 EmberLite 接入 openember-msgs/nanopb，节点注册表可以作为本地运行时视图，消息层只负责把 `NodeInfo`、`NodeHeartbeat` 等协议对象转换进来。
