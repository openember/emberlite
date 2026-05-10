# EmberLite 第三方库管理

与 [OpenEmber 第三方管理说明](../../openember/docs/third-party-management.md) 对齐：**版本与下载 URL 的唯一事实来源**为 [`cmake/Dependencies.cmake`](../cmake/Dependencies.cmake)；离线缓存目录默认为仓库根下 [`third_party/`](../third_party/)。

## 模式

- **FETCH**：优先使用 `third_party/<cache-key>.tar.gz`，否则下载到 `third_party/` 再解压到 `build/_deps/<上游顶层目录>/`。
- **VENDOR**：仅使用 `third_party/` 已有归档或 `*_LOCAL_SOURCE` 指向的已解压目录（不联网）。
- **SYSTEM**：使用系统已安装的包（`find_path` / `find_library` / `pkg-config`）。

Kconfig：`Third-party` 菜单（[`third_party/Kconfig`](../third_party/Kconfig)）选择模式；`scripts/kconfig/genconfig.sh` 生成 `build/config.cmake`。

## 已接入的库（首批）

| 库 | 版本变量 | Get 脚本 | CMake 聚合目标（启用时） |
|----|----------|----------|---------------------------|
| [nng](https://github.com/nanomsg/nng) | `OPENEMBER_NNG_VERSION` | `cmake/GetNng.cmake` | `emberlite::nng` |
| [lcm](https://github.com/lcm-proj/lcm) | `OPENEMBER_LCM_VERSION` | `cmake/GetLcm.cmake` | `emberlite::lcm`（自动链接 `glib-2.0`） |
| [zenoh-pico](https://github.com/eclipse-zenoh/zenoh-pico) | `OPENEMBER_ZENOHPICO_VERSION` | `cmake/GetZenohPico.cmake` | `emberlite::zenohpico` |

在 menuconfig 中勾选 **Libraries (build into this project)** 下的对应项后，运行 `genconfig.sh`，再配置 CMake。

## 为何使用 ExternalProject（与 OpenEmber 的差异说明）

多个上游工程在同一顶层 `project()` 内 `add_subdirectory` 时，可能出现 **全局 CMake target 名称冲突**（例如 nng 与 lcm 都定义 `dist`）。EmberLite 对 **FETCH/VENDOR** 路径采用 **独立前缀的 ExternalProject + IMPORTED 静态库**，将各库安装到 `build/_deps-install/<name>/`，再通过 `emberlite::*` 的 `INTERFACE` 目标暴露给应用。

OpenEmber 若同时启用多个同类依赖，也可能遇到同类限制；EmberLite 侧优先保证 **可同时启用 nng + lcm + zenoh-pico**。

## 本地源码覆盖

在 `cmake/Dependencies.cmake` 中可设置：

- `OPENEMBER_NNG_LOCAL_SOURCE`
- `OPENEMBER_LCM_LOCAL_SOURCE`
- `OPENEMBER_ZENOHPICO_LOCAL_SOURCE`

## 升级

1. 修改 `cmake/Dependencies.cmake` 中的版本与 URL。
2. 删除 `third_party/` 中对应旧归档（如有）并清理 `build/` 后重新配置。
