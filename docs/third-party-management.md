# 第三方依赖管理（EmberLite）

EmberLite 沿用 OpenEmber 的命名与 CMake 约定，便于后续对齐完整框架。当前仓库为**最小迁移**：已具备 **Kconfig → `build/.config` → `build/config.cmake`**、`third_party/` 缓存目录与 `cmake/ThirdPartyArchive.cmake` 等基础设施；具体 **Get\*.cmake** 与上游包可在需要时再接入。

## 模式：`OPENEMBER_THIRD_PARTY_MODE`

由根目录 **Kconfig**（`third_party/Kconfig`）与 `scripts/kconfig/genconfig.sh` 写入 **`build/config.cmake`**，对应 CMake 缓存变量 **`OPENEMBER_THIRD_PARTY_MODE`**：

| 模式 | 含义 |
|------|------|
| **FETCH** | 将来接入 bundle 时，可从固定 URL 下载到 `third_party/` 再解压到 `build/_deps/`（见 `cmake/ThirdPartyArchive.cmake`）。 |
| **VENDOR** | 仅使用已放入 `third_party/` 的归档或本地源码树，不主动联网下载。 |
| **SYSTEM** | 优先/仅使用系统已安装的包（与企业或 Yocto 场景对齐；具体目标库需在对应 Get 脚本中实现）。 |

当前 EmberLite **未**在 `cmake/Dependencies.cmake` 中 `include` 任何 Get 脚本，因此配置阶段不会因缺包而拉取大型依赖；模式变量主要为后续扩展保留。

## 缓存目录：`third_party/`

- 约定与 OpenEmber 一致：将 `<package>-<version>.tar.gz` 或 `.zip` 放在 **`${CMAKE_SOURCE_DIR}/third_party`**（或覆盖缓存变量 **`OPENEMBER_THIRD_PARTY_CACHE_DIR`**）。
- 目录已纳入仓库占位（`.gitkeep`），便于离线或 VENDOR 流程落库归档。

## 与 Kconfig / `ember` 脚本的关系

1. 在仓库根目录执行 **`./scripts/ember menuconfig`**（或 **`OPENEMBER_KCONFIG_NONINTERACTIVE=1`** 生成默认 `.config`）。
2. **`./scripts/ember genconfig`** 根据 `build/.config` 生成 **`build/config.cmake`**。
3. **`cmake -S . -B build`** 会 **`include(build/config.cmake OPTIONAL)`**，从而应用 `OPENEMBER_THIRD_PARTY_MODE` 等开关。

推荐一站式命令：**`./scripts/ember build`**（在无 `.config` / `config.cmake` 时会自动生成默认配置并配置、编译）。

## `OPENEMBER_THIRD_PARTY_BUNDLE_*`（预留）

`cmake/OpenEmberThirdPartyBundleDefaults.cmake` 会为若干 **`OPENEMBER_THIRD_PARTY_BUNDLE_<NAME>`** 提供默认布尔值（与 OpenEmber 一致），供将来接入 ZLOG、NNG 等 bundle 时使用。当前未连接具体 Fetch 逻辑时，这些变量仅存在于 CMake 缓存中，不影响链接产物。

## 与本机缓存目录

- **`third_party/`**：归档落库目录（可提交占位，见 `third_party/.gitkeep`）。
- **`.kconfig-frontends/`**：`scripts/kconfig/ensure-kconfig-frontends-nox.sh` 自动下载的 kconfig 工具，**已加入 `.gitignore`**。

## 参考

- OpenEmber 上游文档与完整依赖链见本仓库内 **`openember/`**（若通过软链接或子模块引入）。
- 构建入口与 Kconfig 流程见 **[构建说明（build.md）](./build.md)**。
