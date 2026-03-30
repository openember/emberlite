# 构建说明（Build）

本项目使用 **CMake** 作为构建系统，当前阶段主要目标：

- 将 `platform/hal` 编译为库（`emberlite_hal`）
- 在 `apps/examples` 构建示例程序
- 所有最终可执行文件统一输出到构建目录的 `bin/` 下

## 依赖

- Linux (POSIX)
- CMake >= 3.16
- GCC/Clang（支持 C11）

## 配置与编译

在项目根目录执行：

```bash
mkdir -p build
cmake -S . -B build
cmake --build build -j
```

编译产物位置：

- 可执行文件：`build/bin/`
- 静态/动态库：`build/lib/`

## 运行示例：串口终端（picocom-like）

示例程序名：`hal_uart_term`

```bash
./build/bin/hal_uart_term -d /dev/ttyUSB0 -b 115200
```

可选参数：

- `--databits 5..8`
- `--stopbits 1|2`
- `--parity none|odd|even`
- `--flow none|hw|sw`

退出方式：

- `Ctrl-A` 然后 `Ctrl-X`

## 常见问题

### 串口权限不足

若打开 `/dev/ttyUSB0` 失败，常见原因是用户不在 `dialout` 组。

```bash
sudo usermod -aG dialout $USER
```

重新登录后生效。

