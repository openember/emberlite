add_executable(hal_uart_term
    uart_term.c
)

target_link_libraries(hal_uart_term PRIVATE emberlite_hal)
target_compile_options(hal_uart_term PRIVATE -Wall -Wextra -Wpedantic)

