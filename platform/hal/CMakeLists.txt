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

find_package(ALSA QUIET)
if(ALSA_FOUND)
    target_compile_definitions(emberlite_hal PRIVATE HAL_HAS_ALSA=1)
    target_link_libraries(emberlite_hal PRIVATE ${ALSA_LIBRARIES})
    target_include_directories(emberlite_hal PRIVATE ${ALSA_INCLUDE_DIRS})
endif()

find_package(PkgConfig QUIET)
if(PkgConfig_FOUND)
    pkg_check_modules(LIBUSB QUIET libusb-1.0)
endif()
if(LIBUSB_FOUND)
    target_compile_definitions(emberlite_hal PRIVATE HAL_HAS_LIBUSB=1)
    target_link_libraries(emberlite_hal PRIVATE ${LIBUSB_LIBRARIES})
    target_include_directories(emberlite_hal PRIVATE ${LIBUSB_INCLUDE_DIRS})
endif()

target_compile_definitions(emberlite_hal PRIVATE _DEFAULT_SOURCE)
target_compile_options(emberlite_hal PRIVATE -Wall -Wextra -Wpedantic)

