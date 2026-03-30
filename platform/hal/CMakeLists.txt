add_library(emberlite_hal
    src/hal.c
    src/hal_uart.c
)

target_include_directories(emberlite_hal
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/include
)

find_package(Threads REQUIRED)
target_link_libraries(emberlite_hal PRIVATE Threads::Threads)

target_compile_options(emberlite_hal PRIVATE -Wall -Wextra -Wpedantic)

