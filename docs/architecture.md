# EmberLite 架构设计规范（v0.1.0）



## 项目定位

EmberLite 是一个面向嵌入式 Linux 类设备的轻量级纯 C runtime 框架，是 OpenEmber 的轻量级同系列产品，目标应用场景包括：

- 机器人系统
- 工业设备
- 科学仪器
- 边缘计算设备
- IoT 网关

核心设计目标：

- 纯 C runtime 核心实现
- 行为可预测（deterministic behavior）
- 依赖极少
- 强模块化结构
- 支持跨进程与跨主机通信
- 面向 10 年以上生命周期的长期可维护性

非目标（v0.1 阶段）：

- 不以 MISRA 合规为目标
- 不支持裸机 MCU 平台
- 不实现 DDS 级 QoS 抽象体系
- 不设计通用 transport 抽象层



## 系统总体架构

EmberLite 采用分层 runtime 架构：

```bash
应用层
   ↓
Node Runtime
   ↓
Executor 调度层
   ↓
消息层
   ↓
通信传输层
   ↓
操作系统（Linux / POSIX）
```

核心设计原则：

```bash
数据面（Data Plane）   → LCM
控制面（Control Plane） → NNG
```

该结构提供：

- 高效数据流通信能力
- RPC 控制能力
- 自动发现能力
- 强调试支持能力
- 极低系统复杂度



## 核心 Runtime 组件

### ember_node

Node 是 EmberLite 的基本执行单元。

职责包括：

- 注册 subscription
- 注册 publisher
- 注册 timer
- 注册 RPC endpoint
- 绑定 executor

生命周期示例：

```c
node_create()
node_init()
node_spin()
node_destroy()
```

注意：Node 不是进程（process），一个进程可以包含多个 node。

设计约束：node 必须保持轻量。

node 的本质是逻辑容器，而不是运行时实体。



## Executor（核心调度引擎）

Executor 负责调度所有回调执行。

管理对象包括：

- subscription callbacks
- timer callbacks
- RPC callbacks
- 内部 runtime 事件

支持的 executor 模型：

### 单线程 executor（默认模式）

```c
while (running)
    poll_events()
    dispatch_callbacks()
```

适用目标：

- 嵌入式 Linux
- 控制系统
- 实时性敏感系统

优势：

- 行为可预测
- 无锁或低锁设计
- 易调试

### 线程池 executor（可选模式）

主线程负责事件轮询，工作线程负责执行回调。

适用目标：

- 边缘计算设备
- 高吞吐机器人系统

### 实时 executor（未来扩展）

计划支持：

- SCHED_FIFO
- CPU 亲和性绑定
- 优先级调度队列



## 消息通信架构

### 通信模型概览

EmberLite 采用双通信平面模型：

```bash
数据流通信 → LCM
控制通信 / RPC → NNG
```

优势：

- 避免 transport 抽象反模式
- 保持性能最优
- 降低系统复杂度
- 最大化工具链兼容性



## 数据通信层（LCM 集成）

LCM 是 EmberLite 的主 publish-subscribe 通信骨架。

职责包括：

- topic 通信
- multicast 传输
- 消息序列化
- 自动发现
- 日志记录
- 回放支持
- 消息可视化支持

使用组件包括：

```bash
liblcm
lcm-gen
```

目标设备 runtime 不需要：

```bash
Java 工具链
Python bindings
C++ bindings
```



### LCM IDL 使用规范

消息定义目录结构：

```bash
msg/
 ├── imu.lcm
 ├── odom.lcm
 └── motor_cmd.lcm
```

生成代码：

```bash
imu.h
imu.c
```

自动生成接口包括：

```c
encode()
decode()
publish()
subscribe()
```



### LCM 传输优势

LCM 提供：

- 自动 discovery
- multicast 通信
- 跨进程通信
- 跨主机通信
- 日志记录能力
- 回放能力

无需实现以下模块：

```
discovery 协议
serialization 层
topic registry
```



## 控制通信层（NNG 集成）

NNG 提供 RPC 能力与控制通信能力。

支持模式包括：

```
REQ/REP
SURVEYOR/RESPONDENT
PAIR
```

主要应用场景：

- 参数服务
- 设备配置
- 诊断查询
- 服务调用
- 远程管理

NNG 是 LCM 的补充，而不是替代。

### RPC 通信模型示例

请求流程：

```
client → request
server → response
```

典型接口：

```c
get_param()
set_param()
query_status()
```



## 定时器子系统

Timer 提供周期性调度能力。

支持模式：

- 周期定时器
- 单次定时器

通过 executor 集成调度：

```c
executor_add_timer()
```

目标精度：< 1ms jitter（建议 RT Linux）



## 参数服务系统

参数服务基于 NNG RPC 实现。

支持接口：

```
get
set
list
subscribe
```

示例接口：

```c
param_get("robot.speed")
param_set("robot.speed", value)
```



## Launch 启动系统

Launch 负责多节点启动编排。

职责包括：

- 进程启动管理
- node 配置注入
- 参数加载
- 命名空间管理

未来支持格式：

- XML
- YAML
- Lua

v0.1 目标：YAML



## 日志与回放支持

通过 LCM 原生工具提供：

```
lcm-logger
lcm-logplayer
```

优势：无需自研日志系统

支持能力：

- 离线调试
- 仿真测试
- 回归测试



## Discovery 机制设计

LCM 提供自动 multicast discovery。

NNG discovery 策略：显式 endpoint 配置

未来可选扩展：UDP broadcast discovery helper

v0.1 暂不强制实现。



## 第三方依赖管理模型

EmberLite 支持三种依赖管理模式：

### FETCH 模式

行为逻辑：

- 优先使用 third_party/
- 否则 FetchContent 自动下载
- 解压至 build/_deps/

适用用户：

- 开源开发者
- 快速原型验证

### VENDOR 模式

行为逻辑：

- 使用 third_party/ 本地归档
- 或 *_LOCAL_SOURCE 指定源码路径

适用用户：

- 离线环境
- 受监管环境
- 可复现构建环境

### SYSTEM 模式

行为逻辑：

- 使用系统安装包
- find_package()

失败行为：

- 配置阶段报错

适用用户：

- Yocto
- 企业 Linux
- CI/CD 环境



## 第三方依赖策略

必须依赖：

- LCM
- NNG

可选依赖：

- YAML parser
- HTTP client
- MQTT client

强约束原则：runtime 必须保持纯 C

禁止依赖：

- Boost
- DDS runtime
- CORBA
- JVM runtime



## Runtime 推荐目录结构

推荐结构如下：

```bash
emberlite/
 ├── runtime/
 ├── executor/
 ├── messaging/
 │    ├── topic_bus/
 │    └── rpc_bus/
 ├── timer/
 ├── param/
 ├── launch/
 └── third_party/
      ├── lcm/
      └── nng/
```



## Transport 抽象策略（重要设计约束）

EmberLite 明确禁止设计通用 transport 抽象层。

禁止结构：

```
transport = LCM | NNG | DDS | ZeroMQ
```

原因：

- discovery 模型不同
- QoS 模型不同
- 线程模型不同

推荐结构：

```
LCM = topic bus
NNG = RPC bus
```

职责明确划分。



## 并发模型设计

Executor 负责统一并发调度。

支持模式：

- 单线程模式
- 多线程模式
- 实时模式（未来）

不推荐应用层自行创建线程。

推荐模式：**统一由 executor 管理调度**



## IPC 通信策略

LCM 支持：

- UDP multicast

NNG 支持：

- IPC sockets
- TCP
- inproc

推荐使用策略：

- 同进程 → inproc
- 同主机 → IPC
- 跨主机 → TCP



## 跨主机通信能力

支持方式：

- LCM multicast
- NNG TCP

无需中心 broker。

无中心架构优势：

- 高鲁棒性
- 高扩展性
- 高容错能力



## 调试工具体系

LCM 工具链支持工具包括：

- lcm-spy
- lcm-logger
- lcm-logplayer

支持能力：

- 消息结构查看
- 频率监控
- 数据解析

三者构成了 runtime observability system，类似于 Wireshark + rosbag + rosbag play

| 工具          | 类型          | 作用                |
| ------------- | ------------- | ------------------- |
| lcm-spy       | 调试 GUI 工具 | 实时查看 topic 数据 |
| lcm-logger    | 数据录制进程  | 记录消息流          |
| lcm-logplayer | 数据回放进程  | 重放消息流          |

lcm-logger 是一个独立进程，它会订阅所有 topic，然后写入 .lcm log 文件。结构类似 rosbag，记录 timestamp、channel、payload 等信息。



## 未来扩展模块规划

可选扩展模块包括：

- DDS bridge
- ROS2 bridge
- MQTT gateway
- HTTP control server
- Web dashboard integration

扩展原则：核心 runtime 始终保持纯 C



## 长期稳定性设计策略

核心保障策略：

- 稳定 C ABI
- 极小依赖集合
- 模块化扩展能力
- 通信职责清晰划分

生命周期目标：10 年以上可维护性



## 最终架构模型总结

EmberLite runtime 总体结构：

```bash
Application
   ↓
  Node
   ↓
Executor
   ↓
LCM topic bus
NNG RPC bus
   ↓
POSIX runtime
```

核心设计优先级：

1. 简单性
2. 可预测性
3. 可移植性
4. 可观测性
5. 可维护性



## EmberLite runtime C API 设计

包含：

① Node API

例如：

```c
ember_node_create()
ember_node_destroy()
ember_node_spin()
```

② Executor API

例如：

```c
ember_executor_create()
ember_executor_add_node()
ember_executor_spin()
```

③ Parameter API

例如：

```c
ember_param_declare()
ember_param_get()
ember_param_set()
```

目标：形成 `include/emberlite/*.h` 级别规范接口设计。



## 核心概念

在 C 语言中，我们不能像 C++ 那样通过继承 `rclcpp::Node` 来创建节点。我们需要更“C 风格”的设计。

### Node（节点） -> `ember_node_t`

在 ROS2 中，Node 是资源的容器。在 EmberLite 中，Node 应该是一个**不透明的句柄**。

- 设计思路：

  - 一个 Node 对应一个 NNG Socket。
  - 利用 NNG 的 `Context` 特性，实现多线程安全的消息处理。

- API 构想：

  ```c
  // 创建节点，指定协议 (如 PUB, SUB, REP)
  ember_node_t* ember_node_create(const char* name, ember_proto_t proto);
  
  // 注册消息回调 (类似 ROS2 的 create_subscription)
  void ember_node_subscribe(ember_node_t* node, const char* topic, ember_msg_callback_t cb);
  
  // 发布消息
  void ember_node_publish(ember_node_t* node, const char* topic, void* data, size_t size);
  ```



#### Node 的本质：进程 vs 线程？

**Node 是一个逻辑概念（句柄），它既可以是进程，也可以是线程，且内部应当支持多线程。**

在 C 语言中，我们不要过度封装，要保留灵活性。

- **逻辑视角：** Node 就是一个 `ember_node_t` 句柄。
- 物理视角（部署模式）：
  - **单进程多线程模式（推荐）：** 在资源受限的嵌入式设备上，通常只运行一个 `robot_app` 进程。在这个进程里，有 `SensorNode`、`ControlNode`、`LoggerNode`。它们共享内存，通过 **NNG Inproc（进程内传输）** 通信，零拷贝，速度极快。
  - **多进程模式：** 在高性能计算单元（如 Jetson Orin）上，视觉算法可能是一个独立的进程，底盘控制是另一个进程。它们通过 **TCP/IPC** 通信。

**关于多线程并发发送：**

- **问题：** 如果一个 Node 内部有 3 个工作线程，都要发消息，怎么办？
- **NNG 的解决方案：** NNG 的 Socket 是**线程安全**的。
- **EmberLite 设计：** 你的 `ember_node_t` 内部持有一个 NNG Socket。多个线程同时调用 `ember_node_publish()` 时，NNG 会在内核层面做锁和消息排队，保证数据不乱序、不冲突。你不需要自己写锁。



### Package（包） -> 目录结构与 Manifest

ROS2 的 Package 依赖 `ament` 或 `catkin` 构建系统。EmberLite 应该更简单，直接基于文件系统。

- Ember Package 结构：

  ```bash
  my_robot_driver/          # 包名
  ├── ember.yaml            # 清单文件 (类似 package.xml，但更简单)
  ├── include/              # 公共头文件
  │   └── my_robot/
  │       └── driver.h
  ├── src/                  # 源码
  │   └── driver.c
  └── CMakeLists.txt        # 或者简单的 Makefile
  ```

- `ember.yaml` 内容示例：

  ```yaml
  name: my_robot_driver
  version: 1.0.0
  # 声明依赖的其他 Ember 包
  dependencies:
    - ember_std_msgs
    - ember_sensors
  # 声明该包提供的节点
  nodes:
    - name: motor_ctrl
      entry: motor_ctrl_main
  ```

#### Package 与 Node 的规划（以机器人为例）

**Package 是“代码仓库/编译单元”，Node 是“运行时实例”。**

- **Package (包)：** 它是**功能模块的集合**。它包含了头文件、源码、配置文件。一个 Package 可以编译出多个 Node，也可以只编译出一个库供其他 Package 使用。
- **Node (节点)：** 它是**执行具体任务的线程/进程**。

**实战案例：一个移动底盘机器人**

| Package 名称         | 描述                                                     | 包含的 Nodes (运行时)                                        |
| :------------------- | :------------------------------------------------------- | :----------------------------------------------------------- |
| **`ember_msgs`**     | **纯数据定义包**。包含所有消息的结构体定义和序列化代码。 | (无，仅提供库)                                               |
| **`drivers_uart`**   | **底层驱动包**。封装串口读写逻辑。                       | (无，作为库被引用)                                           |
| **`robot_chassis`**  | **底盘功能包**。                                         | 1. **`motor_ctrl`**: 读写电机寄存器，发布轮速。<br/> 2. **`odom_estimator`**: 订阅轮速，计算里程计。 |
| **`sensors_lidar`**  | **雷达功能包**。                                         | 1. **`lidar_driver`**: 采集雷达数据，发布点云。              |
| **`app_navigation`** | **顶层应用包**。                                         | 1. **`navigator`**: 订阅里程计和点云，发布速度指令。         |

依赖关系：`app_navigation` 依赖 `robot_chassis` 和 `sensors_lidar` 提供的消息类型。

又或者，我们还是以 **AGV 小车** 为例，按 **功能内聚性** 划分 Package：

| Package        | 职责                           | 部署策略                | Node 示例                      |
| :------------- | :----------------------------- | :---------------------- | :----------------------------- |
| `drivers_gpio` | 底层硬件驱动（PWM、编码器）    | 静态链接到 `robot_core` | (无独立 Node)                  |
| `robot_core`   | **核心调度**（底盘运动学解算） | 单进程 + 多线程         | `odom_estimator`, `motor_ctrl` |
| `sensors_imu`  | IMU 数据采集与滤波             | 单进程（独立线程）      | `imu_driver`                   |
| `app_nav`      | 高层导航逻辑                   | 独立进程（可跨设备）    | `navigator`, `planner`         |

说明：

- `drivers_gpio` 无独立 Node：它只是 `robot_core` 的底层依赖库
- `robot_core` 多线程：里程计计算与电机控制需 *低延迟同步*，共享内存比进程通信快 100 倍
- `app_nav` 独立进程：可在 Jetson 上运行，与 `robot_core` 通过 NNG TCP 通信



### Message（消息） -> 纯 C 结构体 + IDL

ROS2 使用 `.msg` 文件生成 C++ 代码。EmberLite 需要一套轻量级的序列化方案。

- **挑战**：C 语言没有反射。
- 解决方案：
  - **方案 A（简单）**：直接使用 C 结构体，约定内存布局（注意对齐），通过 `memcpy` 传输。适合单机或同构系统。
  - **方案 B（推荐）**：集成 **FlatBuffers** 或 **Cap'n Proto**（C 语言版）。它们是无头（Header-only）的 C 库，零拷贝，非常适合嵌入式。
  - **方案 C（折中）**：自己写一个简单的 IDL 生成器，生成 `.h` 和 `.c` 文件，包含序列化/反序列化函数。



#### 消息序列化设计（以机器人为例）

在 C 语言中，**不要直接 `memcpy` 结构体**（因为不同编译器、不同 CPU 架构的内存对齐不一样）。你需要一套明确的序列化格式。

推荐使用 **FlatBuffers** (零拷贝，极快) 或 **CLANG 结构体 + 手动序列化**。为了演示清晰，这里用**类 C 的结构体定义**作为例子（类似于 ROS2 的 `.msg` 编译后的样子）。

##### 例子 A：传感器数据（高频、实时性要求高）

**消息名：** `Odometry` (里程计)
**场景：** 电机节点 -> 导航节点 (频率 50Hz)

```c
// 定义消息结构 (类似 .msg 文件)
typedef struct {
    double timestamp;   // 时间戳
    double x;           // X 坐标
    double y;           // Y 坐标
    double theta;       // 航向角
    float linear_vel;   // 线速度
    float angular_vel;  // 角速度
} ember_msg_odom_t;

// 序列化后的二进制流大小应该是固定的，例如 32 字节
// 这样 NNG 传输时非常高效
```

##### 例子 B：控制指令（低延迟、可靠性要求高）

**消息名：** `MotorCommand` (电机控制)
**场景：** 导航节点 -> 电机节点 (使用 NNG REQ/REP 或 PUB/SUB)

```c
typedef struct {
    int32_t left_pwm;   // 左轮 PWM (-1000 到 1000)
    int32_t right_pwm;  // 右轮 PWM
    uint8_t mode;       // 模式：0=停止, 1=速度控制, 2=位置控制
    uint8_t reserved[3]; // 补齐字节，保证结构体大小是 4 的倍数
} ember_msg_motor_cmd_t;
```

##### 例子 C：大数据流（非结构化数据）

**消息名：** `ImageRaw` (图像)
**场景：** 摄像头节点 -> 视觉节点 (建议使用 LCM 传输)

```c
typedef struct {
    uint64_t timestamp;
    uint32_t width;
    uint32_t height;
    uint32_t data_size; // 数据长度
    // 后面紧跟 data_size 长度的字节流
    // uint8_t data[data_size]; 
} ember_msg_image_t;
```

### 总结建议

1. **通信：** 核心用 **NNG**，大数据流外挂 **LCM**。
2. **Node：** 设计为**线程安全**的句柄，支持在单进程内多线程并发。
3. **Package：** 按**功能模块**划分（驱动、算法、应用），不要按硬件划分。
4. **消息：** 严格定义结构体对齐，或者引入 FlatBuffers 进行零拷贝序列化。

按照这个思路设计，EmberLite 将会是一个非常扎实、现代化的 C 语言机器人框架。



## 消息通信

### 通信后端选型（NNG vs. LCM）

采用 **“NNG（Nanomsg Next Generation）为主，LCM（Lightweight Communications and Marshalling）为辅（或作为插件）”** 的策略。

**为什么首选 NNG？**

- 协议丰富度：NNG 不仅仅是传输数据，它实现了应用层协议。
  - **REQ/REP**：非常适合嵌入式场景下的“配置下发”或“状态查询”（类似 ROS2 的 Service）。
  - **PUB/SUB**：对应 ROS2 的 Topic，NNG 支持消息过滤，这在带宽受限的嵌入式网络中非常关键。
  - **PIPELINE**：适合做负载均衡（比如多个图像处理节点，分发给空闲的一个）。
- **传输层灵活**：NNG 支持 TCP、IPC（进程间通信）、甚至 Inproc（进程内零拷贝），这意味着你的 EmberLite 程序既可以跨网络通信，也可以在同一个进程内高效交互。

**LCM 的定位**

- LCM 极其轻量，专为**高带宽、低延迟**的流数据设计（如摄像头图像、雷达点云）。
- **建议**：将 LCM 作为一个**可选的传输插件**。默认通信走 NNG，但如果用户需要传高清图像，可以配置走 LCM 通道。

**抽象层**

虽然依赖 NNG/LCM，但建议在框架内部做一个轻量级的消息总线抽象层。

### 控制面和数据面

NNG 和 LCM 是共存关系，不是二选一的竞争关系。

- NNG（控制面 & 通用数据）：
  - **角色：** 系统的“神经系统”。
  - **用途：** 负责指令下发（REQ/REP）、参数配置、日志传输、低频状态广播（PUB/SUB）。
  - **理由：** NNG 的协议语义非常丰富，且支持多种传输介质（TCP, IPC, Inproc），非常适合构建稳健的控制网络。
- LCM（数据面 & 高速流）：
  - **角色：** 系统的“视觉/感知管道”。
  - **用途：** 专门负责高带宽、低延迟的数据流，如摄像头图像、激光雷达点云、IMU 高频数据。
  - **理由：** LCM 基于 UDP 组播，极其轻量，没有连接建立的握手开销，非常适合“发后不管”的大数据流。

设计建议：在 EmberLite 中，**默认集成 NNG**。LCM 作为一个**可选插件**。如果用户的机器人有摄像头，才开启 LCM 支持。



### 跨机通信

对于 EmberLite 这样的嵌入式框架来说，跨机通信（Inter-machine Communication）是核心刚需。NNG 和 LCM 两者都完美支持跨机通信，但它们的**设计哲学**和**适用场景**截然不同。

针对“机器人连接遥控器”和“多台机器人联动”场景，分析如下：

#### NNG

**定位：** 高性能、通用的**套接字库**（Socket Library）。

**跨机能力：** ⭐⭐⭐⭐⭐ (原生支持)

- **通信机制：** NNG 本质上是 BSD 套接字的现代化封装。它原生支持 **TCP**（跨网通信）、**IPC**（本机通信）、**TLS**（加密通信）。
- 针对你的场景：
  - **机器人 <-> 遥控器：** 使用 NNG 的 **TCP** 传输。你可以配置为“请求/回复”模式（类似 RPC，遥控器发指令，机器人回状态）或“发布/订阅”模式（遥控器发控制流，机器人收流）。
  - **多台机器人联动：** 使用 NNG 的 **总线模式（Bus）** 或 **发布/订阅模式**。机器人 A 可以广播自己的位置，机器人 B 和 C 订阅该话题。
- 核心优势：
  - **极其灵活：** 它不局限于某种协议，就像写网络程序一样自然。
  - **性能极高：** 采用无代理设计，消息直接在端点间传输，延迟极低（比 RabbitMQ 低 45%）。
  - **语言绑定丰富：** C/C++（原生）、Python、Go、Rust 等都有很好的支持，适合异构系统。

#### LCM

**定位：** 专为**实时系统**和**机器人**设计的**消息传递中间件**。

**跨机能力：** ⭐⭐⭐⭐ (通过 UDP 组播)

- **通信机制：** LCM 的核心是基于 **UDP 组播**。它的设计初衷就是为了让多个节点（进程或机器）能自动发现彼此并共享数据。
- 针对你的场景：
  - **机器人 <-> 遥控器：** LCM 非常适合。遥控器只需向网络发送 `ControlCommand` 消息，局域网内的所有机器人都能收到。
  - **多台机器人联动：** 这是 LCM 的**杀手级场景**。它支持“发现机制”，机器人启动后自动加入组播组，无需像 NNG 那样手动配置 IP 地址和端口连接。
- 核心优势：
  - **开发体验极佳：** 提供 `lcm-gen` 工具，你定义好数据结构（.lcm 文件），它自动生成 C/C++/Java/Python 代码，保证多语言间数据结构严格对齐。
  - **可视化调试：** LCM 自带 `lcm-logger` 和可视化工具，可以录制和回放机器人数据，这对调试算法至关重要。
  - **低延迟：** 专为实时性优化，头部开销极小。

对比表：

| 特性         | **NNG**                                   | **LCM**                                      |
| :----------- | :---------------------------------------- | :------------------------------------------- |
| **核心传输** | TCP / IPC / TLS                           | UDP Multicast（组播）                        |
| **网络配置** | 需要指定 IP/端口（或通过 DNS）            | **自动发现**（零配置，只要在同一网段）       |
| **可靠性**   | **高**（TCP 保证消息不丢失，支持重连）    | **低**（UDP 尽力而为，丢包不重传，但速度快） |
| **数据定义** | 需自行序列化（JSON/Protobuf/Flatbuffers） | **内置 IDL** (自动代码生成，强类型）         |
| **调试工具** | 需依赖第三方抓包工具                      | **自带录制/回放/查看工具**                   |
| **适用场景** | 需要可靠控制、跨公网、复杂拓扑            | 局域网内高频数据共享、多机协同、传感器数据流 |

对于 EmberLite 项目，考虑到我们是针对嵌入式 Linux 的框架：

1. 如果侧重于“控制”和“可靠性”：选 **NNG**。
   - 例如：遥控器发送“急停”指令，或者配置机器人的静态 IP。TCP 的可靠性是必须的，你不能容忍控制指令丢失。
2. 如果你侧重于“数据流”和“多机协同”：选 **LCM**。
   - 例如：机器人 A 把激光雷达数据、里程计数据广播出去，机器人 B 和遥控器同时接收。LCM 的组播机制在这里比 NNG 的点对点连接要高效且简单得多。

**终极方案（混合架构）**：很多高级机器人系统（如自动驾驶栈）是**同时使用**这两者的：

- 用 **NNG（TCP）** 处理**控制链路**（遥控器 -> 机器人，确保指令必达）。
- 用 **LCM（UDP）** 处理**数据链路**（传感器 -> 算法 -> 日志，追求极致吞吐和低延迟）。

**对于 EmberLite 的一句话推荐：**
如果只能选一个作为框架的基础通信层，我推荐 **NNG**，因为它的通用性更强（支持 TCP/IP/IPC），且更容易封装出类似 LCM 的上层功能；而 LCM 更适合作为一个独立的“数据总线”插件集成进来。

## HAL 硬件抽象层

- **接口标准化**： 定义一套标准的 C 接口（如 `i2c_read`, `gpio_write`），但实现由用户通过函数指针注入。这样你的框架就能在 STM32、ESP32 或 Linux 用户态上无缝运行。



## 脚手架工具（CLI）

功能：能生成标准的目录结构，方便用户进行二次开发（基于本项目创建一个干净的工程）。

```bash
my_project/
├── apps/          # 用户应用
├── modules/       # 自定义模块
├── hal/           # 硬件抽象层
├── core/          # Ember 核心代码
└── build/         # 构建输出
```

这个脚手架工具本身可以用 Python 或 Go 写（作为构建依赖），但生成的 C 代码必须纯净。或者，为了极致轻量，用 Shell 脚本写一个简单的生成器也是 C 程序员喜欢的风格。

EmberLite 只需 **3 个轻量工具**：

| 工具           | 作用                                 | 替代 ROS2 的什么？ | 实现方式     |
| :------------- | :----------------------------------- | :----------------- | :----------- |
| `ember-init`   | 创建新 Package 模板                  | `ros2 pkg create`  | 100 行 Shell |
| `ember-build`  | 递归编译所有依赖的 Package           | `colcon build`     | CMake 驱动   |
| `ember-launch` | **核心！** 按 YAML 描述启动进程/线程 | `ros2 launch`      | Python 脚本  |

`ember-launch` 的魔法：进程/线程的灵活编排

```yaml
# launch.yaml
nodes:
  - name: motor_ctrl
    package: robot_chassis
    entry: chassis_main  # C 函数名
    threads: 2           # 启动 2 个线程共享此 Node
    affinity: "0-1"     # CPU 亲和性（嵌入式关键！）

  - name: lidar_driver
    package: sensors_lidar
    entry: lidar_main
    process: true      # 显式声明为独立进程
```

**工作原理**：`ember-launch` 解析 YAML 后：

- 对 `threads: N` → 在 *当前进程* 内创建 N 个线程，每个线程调用 `chassis_main()`
- 对 `process: true` → 用 `fork()` 启动新进程，执行 `./build/sensors_lidar/lidar_main`



## 核心原则

1. ❌ 不做 ROS2 clone
2. ✅ Node ≠ Socket
3. ✅ 单通信模型优先（NNG）
4. ❌ 不引入多中间件（LCM）
5. ✅ IDL 自研（轻量）
6. ❌ Package 不是必须
7. ✅ Runtime 控制线程，而不是 Node



## 实施建议

1. **从“进程内通信”开始**
   不要一开始就搞网络分布。先实现 **Inproc Transport**（NNG 支持）。让 `Sensor Node` 和 `Control Node` 在同一个 `main()` 函数里跑通，通过 NNG 的管道交换数据。这能验证你的架构是否合理。
2. **设计“消息总线”抽象层**
   不要让用户直接调用 `nng_send`。封装一层 `ember_send`。
   - *理由*：未来如果你想把后端从 NNG 换成 ZeroMQ 或 Shared Memory，用户代码不需要修改。
3. **脚手架工具 (`ember-cli`)**
   既然你要做脚手架，它应该能生成上述的目录结构，并自动生成 `CMakeLists.txt`，把所有依赖的 Ember 包链接起来。



## 相关链接

- https://github.com/nanomsg/nng
- https://github.com/lcm-proj/lcm