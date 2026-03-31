# 在 include(config.cmake) 之后加载：为 Kconfig 未生成的 bundle 开关提供默认 ON（与 OpenEmber 旧行为一致）。
# EmberLite 当前未接入具体 bundle；保留变量名以便后续扩展 Get*.cmake。

foreach(_b
    ZLOG CJSON NLOHMANN_JSON SQLITE MONGOOSE YAMLCPP ASIO SPDLOG PAHO
    LIBZMQ NNG LCM CPPZMQ RUCKIG)
    if(NOT DEFINED OPENEMBER_THIRD_PARTY_BUNDLE_${_b})
        set(OPENEMBER_THIRD_PARTY_BUNDLE_${_b} ON CACHE BOOL
            "Include third-party bundle ${_b} in FETCH/VENDOR (OFF = use system install if found)" FORCE)
    endif()
endforeach()
