# 硬件抽象层（HAL）设计规范 v1.1

## 1. 设计目标与原则

本规范旨在定义一套运行于 Linux 用户空间的纯 C 语言硬件抽象层（HAL），用于统一访问 UART、GPIO、SPI、I2C、CAN 等硬件接口。

- **统一性（Uniformity）**：所有模块遵循相同的命名、错误处理和资源管理风格。
- **可移植性（Portability）**：屏蔽底层驱动差异（如 `/dev/ttyUSB` vs `/dev/ttyS`），便于迁移到其他 Unix-like 系统。
- **线程安全（Thread-Safety）**：所有公开 API 默认线程安全。
- **灵活性（Flexibility）**：同时支持同步阻塞调用和异步事件回调，底层暴露文件描述符以供高级集成。
- **零依赖（Zero Dependency）**：仅依赖标准 C 库（glibc）和 Linux 系统调用，无第三方库依赖。

HAL 的边界（Boundary）：HAL 应该管理“外设（Peripherals）”和“总线（Buses）”，而不应该管理“协议（Protocols）”和“网络服务（Network Services）”。

## 2. 命名规范 (Naming Convention)

所有公开符号必须使用 `hal_` 前缀，避免链接冲突。采用 **小写字母 + 下划线** 风格。

### 2.1 模块前缀

格式：`hal_<module>_<action>`

| 模块         | 前缀        | 示例                                  |
| :----------- | :---------- | :------------------------------------ |
| 通用/核心    | `hal_`      | `hal_init()`, `hal_status_t`          |
| 串口         | `hal_uart_` | `hal_uart_open()`, `hal_uart_read()`  |
| 通用输入输出 | `hal_gpio_` | `hal_gpio_write()`, `hal_gpio_read()` |
| 串行外设接口 | `hal_spi_`  | `hal_spi_transfer()`                  |
| 集成电路总线 | `hal_i2c_`  | `hal_i2c_write_reg()`                 |
| 控制器局域网 | `hal_can_`  | `hal_can_send()`                      |

### 2.2 类型定义

- **句柄**：`hal_<module>_handle_t` ( opaque pointer, e.g., `void*` or `struct*`)
- **配置**：`hal_<module>_config_t` ( struct )
- **状态码**：`hal_status_t` ( enum )
- **回调**：`hal_<module>_callback_t` ( function pointer )

### 2.3 函数分类

- **生命周期**：`_open`, `_close`, `_init`, `_deinit`
- **配置**：`_set_config`, `_get_config`
- **数据 IO**：`_read`, `_write`, `_transfer`, `_send`, `_recv`
- **控制**：`_ioctl`, `_set_control`, `_flush`
- **事件**：`_register_callback`, `_start_async`, `_stop_async`

## 3. 核心类型与错误处理

### 3.1 统一状态码

不直接返回 `-1` 或 `errno`，使用统一枚举。

```c
typedef enum {
    HAL_OK                  = 0,
    HAL_ERR_IO              = 1,    /* 读写错误 */
    HAL_ERR_TIMEOUT         = 2,    /* 操作超时 */
    HAL_ERR_INVALID_PARAM   = 3,    /* 参数错误 */
    HAL_ERR_NOT_SUPPORTED   = 4,    /* 功能不支持 */
    HAL_ERR_NO_MEMORY       = 5,    /* 内存不足 */
    HAL_ERR_BUSY            = 6,    /* 设备忙 */
    HAL_ERR_PERMISSION      = 7,    /* 权限不足 */
    HAL_ERR_NODEVICE        = 8     /* 设备不存在 */
} hal_status_t;
```

### 3.2 时间单位

所有超时参数统一使用 **毫秒（ms）**，类型为 `int32_t`。

- `0` : 非阻塞（立即返回）
- `-1`: 无限阻塞（默认）
- `>0`: 超时时间

### 3.3 数据缓冲

所有数据缓冲统一使用 `uint8_t*`，长度使用 `size_t`。

## 4. 通用接口模式（Common Interface Pattern）

所有硬件模块应尽可能遵循以下生命周期模型。

### 4.1 打开与关闭

```c
/* 打开设备，返回句柄 */
hal_status_t hal_<module>_open(const hal_<module>_config_t* config, hal_<module>_handle_t* handle);

/* 关闭设备，释放资源 */
hal_status_t hal_<module>_close(hal_<module>_handle_t handle);
```

### 4.2 同步 IO（基础能力）

所有支持数据流的模块必须实现同步读写。

```c
/* 读取数据，带超时 */
hal_status_t hal_<module>_read(hal_<module>_handle_t handle, uint8_t* buffer, size_t length, int32_t timeout_ms, size_t* out_read_len);

/* 写入数据，带超时 */
hal_status_t hal_<module>_write(hal_<module>_handle_t handle, const uint8_t* buffer, size_t length, int32_t timeout_ms, size_t* out_write_len);
```

### 4.3 异步/事件支持（高级能力）

为了解决同步阻塞问题，HAL 提供统一的回调注册机制。 **设计哲学**：HAL 内部维护一个事件管理线程（Reactor 模式），用户无需关心线程创建。

```c
/* 定义通用事件回调 */
typedef void (*hal_event_callback_t)(hal_<module>_handle_t handle, uint32_t event_id, void* data, size_t len, void* user_data);

/* 注册回调 */
hal_status_t hal_<module>_register_callback(hal_<module>_handle_t handle, hal_event_callback_t callback, void* user_data);

/* 启用/禁用事件监听 */
hal_status_t hal_<module>_set_event_mask(hal_<module>_handle_t handle, uint32_t mask);
```

### 4.4 底层描述符暴露（集成能力）

允许高级用户将 HAL 句柄集成到自己的 `epoll` 或 GUI 事件循环中。

```c
/* 获取底层文件描述符 (如不支持返回 -1) */
int hal_<module>_get_fd(hal_<module>_handle_t handle);
```

## 5. 模块详细规范

### 5.1 UART（串口）

- **特性**：流式数据，需配置波特率、流控。

- 配置结构体：

  ```c
  typedef struct {
      const char* path;       // "/dev/ttyUSB0"
      uint32_t baudrate;      // 115200
      uint8_t data_bits;      // 8
      uint8_t stop_bits;      // 1
      uint8_t parity;         // 0=None, 1=Odd, 2=Even
      uint8_t flow_control;   // 0=None, 1=HW, 2=SW
  } hal_uart_config_t;
  ```

- 特有 API：

  - `hal_uart_flush(handle)`：清空缓冲区。
  - `hal_uart_set_break(handle, state)`：发送 Break 信号。
  - `hal_uart_set_control(handle, line, state)`：控制 RTS/DTR。

- 事件 ID：

  - `HAL_UART_EVENT_RX`：收到数据。
  - `HAL_UART_EVENT_ERROR`：帧错误/校验错误。

### 5.2 GPIO（通用输入输出）

- **特性**：电平读写，边缘中断。

- 配置结构体：

  ```c
  typedef struct {
      uint32_t pin;           // 引脚号 (或芯片名称 + 偏移)
      uint8_t direction;      // 0=In, 1=Out
      uint8_t pull_mode;      // 0=None, 1=Up, 2=Down
      uint8_t initial_level;  // 输出初始电平
  } hal_gpio_config_t;
  ```

- 特有 API：

  - `hal_gpio_read(handle, uint8_t* level)`
  - `hal_gpio_write(handle, uint8_t level)`
  - `hal_gpio_toggle(handle)`

- 事件 ID：

  - `HAL_GPIO_EVENT_RISING`：上升沿。
  - `HAL_GPIO_EVENT_FALLING`：下降沿。
  - `HAL_GPIO_EVENT_BOTH`：双边沿。

### 5.3 SPI（串行外设接口）

- **特性**：全双工，主从模式，事务性。

- 配置结构体：

  ```c
  typedef struct {
      const char* path;       // "/dev/spidev0.0"
      uint32_t speed_hz;      // 1000000
      uint8_t mode;           // SPI Mode 0-3
      uint8_t bits_per_word;  // 8
  } hal_spi_config_t;
  ```

- 特有 API：

  - `hal_spi_transfer(handle, const uint8_t* tx, uint8_t* rx, size_t len)`：同时收发。
  - `hal_spi_set_cs(handle, state)`：手动控制片选（如需）。

- **注意**：SPI 通常不支持异步流式读取，异步主要指事务完成回调。

### 5.4 I2C（集成电路总线）

- **特性**：半双工，地址寻址，寄存器操作。

- 配置结构体：

  ```c
  typedef struct {
      const char* path;       // "/dev/i2c-1"
      uint16_t slave_addr;    // 从机地址
      uint32_t timeout_ms;    // 总线超时
  } hal_i2c_config_t;
  ```

- 特有 API：

  - `hal_i2c_write_reg(handle, uint8_t reg, const uint8_t* data, size_t len)`
  - `hal_i2c_read_reg(handle, uint8_t reg, uint8_t* data, size_t len)`
  - `hal_i2c_scan(handle)`：扫描总线设备。

### 5.5 CAN（控制器局域网）

- **特性**：报文帧，ID 过滤，网络套接字。

- 配置结构体：

  ```c
  typedef struct {
      const char* ifname;     // "can0"
      uint32_t bitrate;       // 500000
      uint32_t filter_id;     // 过滤 ID (可选)
  } hal_can_config_t;
  ```

- 数据结构：

  ```c
  typedef struct {
      uint32_t id;
      uint8_t dlc;
      uint8_t data[8];
      uint8_t is_rtr;
  } hal_can_frame_t;
  ```

- 特有 API：

  - `hal_can_send(handle, const hal_can_frame_t* frame)`
  - `hal_can_receive(handle, hal_can_frame_t* frame, int32_t timeout_ms)`

### 5.6 PWM（脉冲宽度调制）

- **用途**：电机控制、LED 亮度调节、舵机控制。

- 关键 API：

  ```c
  hal_pwm_set_frequency(handle, hz);
  hal_pwm_set_duty_cycle(handle, percent); // 0.0 ~ 100.0
  hal_pwm_enable(handle);
  ```

### 5.7 ADC（模数转换）

- **用途**：读取电池电压、电位器、光敏电阻。

- 关键 API：

  ```c
  hal_adc_read_raw(handle, &value);      // 读取 0-4095
  hal_adc_read_voltage(handle, &mv);     // 读取毫伏值
  ```

### 5.8 Input（输入设备）

- **用途**：读取按键、编码器、触摸屏、USB 键盘。

- **实现**：封装 Linux `/dev/input/event*` 和 `evdev` 协议。

- 关键 API：

  ```c
  hal_input_read_event(handle, &event); // 返回按键按下/释放、坐标等
  ```

### 5.9 I2S（音频接口）

- **用途**：麦克风、扬声器 (音频数据流)。

- **实现**：Linux ALSA 系统非常复杂，HAL 可简化为简单的读写 PCM 数据流。直接操作 `/dev/snd/` 较复杂，使用 `alsa-lib` 更稳妥。

- 关键 API：

  ```c
  typedef struct {
      const char* device_name;   // "default" 或 "hw:0,0"
      uint32_t sample_rate;      // 44100, 48000
      uint8_t channels;          // 1 (Mono), 2 (Stereo)
      uint8_t bits_per_sample;   // 16, 24, 32
  } hal_i2s_config_t;
  
  hal_status_t hal_i2s_open(const hal_i2s_config_t* cfg, hal_i2s_handle_t* handle);
  hal_status_t hal_i2s_write(hal_i2s_handle_t handle, const uint8_t* buffer, size_t frames, int timeout_ms); // 播放
  hal_status_t hal_i2s_read(hal_i2s_handle_t handle, uint8_t* buffer, size_t frames, int timeout_ms); // 录音
  hal_status_t hal_i2s_start(hal_i2s_handle_t handle);
  hal_status_t hal_i2s_stop(hal_i2s_handle_t handle);
  ```

- **注意**：单位是 **Frame**（一帧包含所有声道的数据），而不是 Byte。例如 16 位立体声，1 Frame = 4 Bytes。

### 5.10 1-Wire（单总线）

- **用途**：连接 DS18B20 温度传感器、MAX31826 等单总线设备。

- **实现方案**：

  - **推荐**：利用 Linux 内核驱动 `w1-gpio`。内核负责微秒级时序，用户态通过 `/sys/bus/w1/devices/` 读取数据。稳定可靠。
  - **不推荐**：用户态 GPIO 位翻转（Bit-banging）。Linux 不是实时系统，时序极易抖动，导致读取失败。

- **关键 API**：

  ```c
  // 配置：通常只需指定总线 ID 或设备序列号
  typedef struct {
      const char* bus_path;      // "/sys/bus/w1/devices/"
      const char* device_id;     // "28-000001234567" (DS18B20 序列号)
  } hal_onewire_config_t;
  
  hal_status_t hal_onewire_open(const hal_onewire_config_t* cfg, hal_onewire_handle_t* handle);
  hal_status_t hal_onewire_read_temp(hal_onewire_handle_t handle, float* temp_c); // 直接读取温度
  hal_status_t hal_onewire_read_raw(hal_onewire_handle_t handle, uint8_t* buf, size_t len); // 读取原始数据
  ```

- **注意**：DHT11 **不是** 标准 1-Wire 协议，时序更苛刻且无内核驱动，建议用 `hal_gpio` 单独写驱动，不要混入 `hal_onewire`。

### 5.11 USB

- **用途**：访问非串口的 USB 设备（如自定义 HID 设备、USB 摄像头控制、U 盘读写、FTDI 特殊控制）。

- **实现方案**：封装 `libusb-1.0` 库。Linux 下 USB 设备文件 (`/dev/bus/usb/...`) 权限复杂且接口底层，`libusb` 是标准做法。

- **关键 API**：

  ```c
  typedef struct {
      uint16_t vendor_id;
      uint16_t product_id;
      int interface_number;
  } hal_usb_config_t;
  
  // 控制传输 (配置设备)
  hal_status_t hal_usb_control_transfer(hal_usb_handle_t handle, uint8_t req_type, uint8_t req, uint16_t val, uint16_t idx, uint8_t* data, size_t len);
  
  // 批量传输 (大数据)
  hal_status_t hal_usb_bulk_write(hal_usb_handle_t handle, uint8_t endpoint, const uint8_t* data, size_t len, int timeout_ms);
  hal_status_t hal_usb_bulk_read(hal_usb_handle_t handle, uint8_t endpoint, uint8_t* data, size_t len, int timeout_ms, size_t* out_len);
  ```

- **注意**：USB 转串口设备（如 CH340）**不应** 使用此模块，应直接使用 `hal_uart` (`/dev/ttyUSB0`)，这样更简单高效。

### 5.12 System（系统信息/服务）

- **用途**：获取 CPU 温度、内存使用、运行时间，设置定时器、看门狗等。

- 关键 API：

  ```c
  /* hal_system.h */
  
  // 1. 时间获取 (替代 HAL 中的时间函数)
  uint64_t hal_system_get_time_ms(void);   // 单调递增时间 (ms)
  uint64_t hal_system_get_time_us(void);   // 单调递增时间 (us)
  uint64_t hal_system_get_uptime_ms(void); // 系统运行时间
  uint64_t hal_system_get_cpu_temp(void);  // CPU温度
  
  // 2. 延时 (阻塞)
  void hal_system_sleep_ms(uint32_t ms);
  
  // 3. 定时器句柄 (基于 timerfd)
  typedef void* hal_timer_handle_t;
  
  typedef struct {
      uint32_t interval_ms;   // 间隔时间
      uint32_t initial_ms;    // 初始延迟
      bool periodic;          // 是否周期触发
  } hal_timer_config_t;
  
  typedef void (*hal_timer_callback_t)(hal_timer_handle_t handle, void* user_data);
  
  // 4. 定时器 API
  hal_timer_handle_t hal_timer_create(const hal_timer_config_t* cfg, hal_timer_callback_t cb, void* user_data);
  hal_status_t hal_timer_start(hal_timer_handle_t handle);
  hal_status_t hal_timer_stop(hal_timer_handle_t handle);
  void hal_timer_destroy(hal_timer_handle_t handle);
  
  // 5. 集成到事件循环 (关键)
  // 允许用户将定时器 FD 加入自己的 epoll
  int hal_timer_get_fd(hal_timer_handle_t handle); 
  
  // 6. 看门狗 - 系统死机自动重启，封装 `/dev/watchdog`
  int hal_watchdog_feed(handle); // 喂狗
  int hal_watchdog_set_timeout(handle, seconds);
  ```

注意事项：

- **统一性**：`hal_timer_get_fd()` 返回的文件描述符可以和 `hal_uart_get_fd()` 一起放入同一个 `epoll` 循环中。
- **非阻塞**：不需要创建额外的线程，由主事件循环统一调度。
- **精准度**：`timerfd` 由内核保证精度，优于 `sleep` + 线程。

## 6. 线程安全与并发设计

### 6.1 内部锁

- 所有句柄内部必须包含互斥锁（`pthread_mutex_t`）。
- **读锁**：允许多个线程同时读（如果底层驱动支持）。
- **写锁**：写入操作必须互斥，防止数据包撕裂。
- **建议**：为简化实现，每个句柄维护一个互斥锁，读写操作前加锁。

### 6.2 异步实现模型

HAL 内部实现一个 **单线程事件反应器（Event Reactor）**：

1. 当用户调用 `register_callback` 时，将该设备的 `fd` 加入内部监控列表。
2. HAL 启动一个后台线程，运行 `epoll_wait` 或 `select` 循环。
3. 当 `fd` 就绪时，后台线程读取数据，并调用用户注册的回调。
4. **优势**：无论用户打开多少个串口/GPIO，HAL 只消耗一个线程资源。

### 6.3 重入保护

- 回调函数中**禁止**再次调用同一句柄的同步读写接口，防止死锁。
- 回调函数应尽快返回，耗时操作请移交业务线程。

## 7. 资源管理与清理

### 7.1 初始化/反初始化

- `hal_init()`: 全局初始化（如启动事件反应线程）。
- `hal_deinit()`: 全局清理（停止线程，关闭所有未关闭句柄）。
- **注意**：如果 HAL 设计为无状态（Stateless），可省略此步骤，但在异步模式下建议保留。

### 7.2 句柄生命周期

- `open` 分配资源，`close` 必须释放所有资源（包括线程、锁、缓冲区）。
- 如果用户忘记 `close`，HAL 应提供 `atexit` 钩子进行清理（可选）。

## 8. 使用示例

### 8.1 同步模式（UART）

```c
hal_uart_handle_t uart;
hal_uart_config_t cfg = { .path = "/dev/ttyUSB0", .baudrate = 115200, ... };
uint8_t buf[32];
size_t len;

if (hal_uart_open(&cfg, &uart) == HAL_OK) {
    hal_uart_write(uart, "Hello", 5, 1000, NULL);
    hal_uart_read(uart, buf, 32, 1000, &len);
    hal_uart_close(uart);
}
```

### 8.2 异步模式（UART + GPIO）

```c
void on_event(hal_uart_handle_t h, uint32_t id, void* data, size_t len, void* userdata) {
    if (id == HAL_UART_EVENT_RX) {
        printf("Recv: %.*s\n", (int)len, (char*)data);
    }
}

// 全局初始化
hal_init(); 

// 打开并注册
hal_uart_handle_t uart;
hal_uart_open(&cfg, &uart);
hal_uart_register_callback(uart, on_event, NULL);
hal_uart_set_event_mask(uart, HAL_UART_EVENT_RX);

// 主循环做其他事
while(1) { sleep(1); }

// 清理
hal_uart_close(uart);
hal_deinit();
```

### 8.3 集成到外部事件循环 (epoll)

```c
int fd = hal_uart_get_fd(uart);
struct epoll_event ev = { .events = EPOLLIN, .data.fd = fd };
epoll_ctl(epfd, EPOLL_CTL_ADD, fd, &ev);

// 用户自己的 epoll_wait 循环中处理
```

## 9. 目录结构建议

```bash
hal_linux/
├── include/
│   ├── hal.h             # 核心类型与错误码
│   ├── hal_uart.h
│   ├── hal_gpio.h
│   ├── hal_spi.h
│   └── ...
├── src/
│   ├── hal_core.c        # 全局初始化，事件反应线程
│   ├── hal_uart.c        # UART 实现
│   ├── hal_gpio.c        # GPIO 实现 (libgpiod 封装)
│   └── ...
├── tests/
│   ├── test_uart_loopback.c
│   └── ...
└── CMakeLists.txt
```

## 10. 附录：开发注意事项

1. **权限问题**：访问 `/dev/tty*` 或 `/dev/gpio*` 通常需要 `dialout` 或 `gpio` 组权限。HAL 文档需注明。
2. **虚拟设备**：确保代码能处理虚拟设备（如 `pty`），便于单元测试。
3. **热插拔**：Linux 下 USB 串口可能随时断开。`read/write` 返回 `HAL_ERR_IO` 时，应用层应尝试重连。
4. **ABI 兼容性**：配置结构体 (`config_t`) 应保留 `reserved` 字段，以便未来扩展而不破坏二进制兼容性。