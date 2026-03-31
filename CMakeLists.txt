cmake_minimum_required(VERSION 3.16)

project(EmberLite LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin")
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_SOURCE_DIR}/cmake")
include(${CMAKE_BINARY_DIR}/config.cmake OPTIONAL)
include(cmake/Dependencies.cmake)

# 未使用 ember/kconfig 时，与原先“全部子目录 + HAL 自动探测”行为一致
if(NOT DEFINED OPENEMBER_ENABLE_HAL)
    set(OPENEMBER_ENABLE_HAL ON CACHE BOOL "Build platform HAL (emberlite_hal)")
endif()
if(OPENEMBER_ENABLE_HAL)
    if(NOT DEFINED OPENEMBER_HAL_ALSA)
        set(OPENEMBER_HAL_ALSA "AUTO" CACHE STRING "HAL ALSA: AUTO|ON|OFF")
        set_property(CACHE OPENEMBER_HAL_ALSA PROPERTY STRINGS AUTO ON OFF)
    endif()
    if(NOT DEFINED OPENEMBER_HAL_LIBUSB)
        set(OPENEMBER_HAL_LIBUSB "AUTO" CACHE STRING "HAL libusb: AUTO|ON|OFF")
        set_property(CACHE OPENEMBER_HAL_LIBUSB PROPERTY STRINGS AUTO ON OFF)
    endif()
    add_subdirectory(platform/hal)
endif()
if(NOT DEFINED OPENEMBER_ENABLE_EXAMPLES)
    set(OPENEMBER_ENABLE_EXAMPLES ON CACHE BOOL "Build apps/examples")
endif()
if(NOT DEFINED OPENEMBER_ENABLE_CRYPTO)
    set(OPENEMBER_ENABLE_CRYPTO ON CACHE BOOL "Build components/crypto")
endif()

if(OPENEMBER_ENABLE_CRYPTO)
    add_subdirectory(components/crypto)
endif()
if(OPENEMBER_ENABLE_EXAMPLES)
    if(NOT OPENEMBER_ENABLE_HAL)
        message(FATAL_ERROR
            "OPENEMBER_ENABLE_EXAMPLES is ON but OPENEMBER_ENABLE_HAL is OFF. "
            "UART examples require emberlite_hal. Disable examples in Kconfig or enable HAL.")
    endif()
    add_subdirectory(apps/examples)
endif()
