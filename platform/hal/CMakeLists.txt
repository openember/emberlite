add_library(emberlite_hal
    src/hal.c
    src/hal_adc.c
    src/hal_can.c
    src/hal_gpio.c
    src/hal_input.c
    src/hal_i2s.c
    src/hal_i2c.c
    src/hal_onewire.c
    src/hal_pwm.c
    src/hal_system.c
    src/hal_usb.c
    src/hal_spi.c
    src/hal_uart.c
)

target_include_directories(emberlite_hal
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/include
)

find_package(Threads REQUIRED)
target_link_libraries(emberlite_hal PRIVATE Threads::Threads)

# OPENEMBER_HAL_ALSA / OPENEMBER_HAL_LIBUSB：AUTO（默认，与旧行为一致）| ON（必选）| OFF（禁用）
if(OPENEMBER_HAL_ALSA STREQUAL "OFF")
    # Kconfig 显式关闭：不探测、不链接
elseif(OPENEMBER_HAL_ALSA STREQUAL "ON")
    find_package(ALSA REQUIRED)
    target_compile_definitions(emberlite_hal PRIVATE HAL_HAS_ALSA=1)
    target_link_libraries(emberlite_hal PRIVATE ${ALSA_LIBRARIES})
    target_include_directories(emberlite_hal PRIVATE ${ALSA_INCLUDE_DIRS})
else()
    find_package(ALSA QUIET)
    if(ALSA_FOUND)
        target_compile_definitions(emberlite_hal PRIVATE HAL_HAS_ALSA=1)
        target_link_libraries(emberlite_hal PRIVATE ${ALSA_LIBRARIES})
        target_include_directories(emberlite_hal PRIVATE ${ALSA_INCLUDE_DIRS})
    endif()
endif()

if(OPENEMBER_HAL_LIBUSB STREQUAL "OFF")
    # 显式关闭
elseif(OPENEMBER_HAL_LIBUSB STREQUAL "ON")
    find_package(PkgConfig REQUIRED)
    pkg_check_modules(LIBUSB REQUIRED libusb-1.0)
    target_compile_definitions(emberlite_hal PRIVATE HAL_HAS_LIBUSB=1)
    target_link_libraries(emberlite_hal PRIVATE ${LIBUSB_LIBRARIES})
    target_include_directories(emberlite_hal PRIVATE ${LIBUSB_INCLUDE_DIRS})
else()
    find_package(PkgConfig QUIET)
    if(PkgConfig_FOUND)
        pkg_check_modules(LIBUSB QUIET libusb-1.0)
    endif()
    if(LIBUSB_FOUND)
        target_compile_definitions(emberlite_hal PRIVATE HAL_HAS_LIBUSB=1)
        target_link_libraries(emberlite_hal PRIVATE ${LIBUSB_LIBRARIES})
        target_include_directories(emberlite_hal PRIVATE ${LIBUSB_INCLUDE_DIRS})
    endif()
endif()

target_compile_definitions(emberlite_hal PRIVATE _DEFAULT_SOURCE)
target_compile_options(emberlite_hal PRIVATE -Wall -Wextra -Wpedantic)
