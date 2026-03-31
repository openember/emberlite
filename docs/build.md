# 构建说明（Build）

本项目使用 **CMake**（≥ 3.16）作为主构建系统，并可选使用与 OpenEmber 一致的 **Kconfig → `build/.config` → `build/config.cmake`** 流程统一管理功能开关与第三方策略。

## 依赖

- Linux (POSIX)
- CMake >= 3.16
- GCC/Clang（支持 C11）

## 推荐：`ember` 一键构建

在**仓库根目录**执行：

```bash
chmod +x ./scripts/ember   # 仅需一次
./scripts/ember build
```

说明：

- 若不存在 **`build/.config`**，脚本会以非交互方式生成默认配置（`OPENEMBER_KCONFIG_NONINTERACTIVE=1`）。
- 根据 `build/.config` 生成 **`build/config.cmake`**，随后执行 `cmake -S -B` 与 `cmake --build`。
- `./scripts/ember menuconfig` 首次运行会自动下载 `kconfig-frontends-nox` 到 **`.kconfig-frontends/`**（需 `curl`、`dpkg-deb`）；该目录已加入 **`.gitignore`**，无需提交。

常用子命令：

| 命令 | 说明 |
|------|------|
| `./scripts/ember menuconfig [build_dir]` | 交互式菜单，写入 `build/.config` 并刷新 `config.cmake` |
| `./scripts/ember genconfig [build_dir]` | 仅根据 `.config` 重写 `build/config.cmake` |
| `./scripts/ember update [build_dir]` | 等价于 `cmake -S . -B build`（应用 `config.cmake`） |
| `./scripts/ember clean [build_dir]` | 删除构建目录（默认 `build/`） |

Bash 补全：`./scripts/ember completion bash`，或由 `./scripts/ember install` 写入 `~/.bashrc` 后自动加载。

注意：若同一台机器上同时有多个工程都执行 `ember install`，全局命令 `ember` 可能会互相覆盖；推荐直接在各自工程根目录使用 `./scripts/ember`。

## 传统：纯 CMake（无 Kconfig）

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

退出方式：`Ctrl-A` 然后 `Ctrl-X`。

## 常见问题

### 串口权限不足

若打开 `/dev/ttyUSB0` 失败，常见原因是用户不在 `dialout` 组。

```bash
sudo usermod -aG dialout $USER
```

重新登录后生效。

