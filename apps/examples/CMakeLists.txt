add_executable(hal_uart_term
    uart_term.c
)

target_link_libraries(hal_uart_term PRIVATE emberlite_hal)
target_compile_options(hal_uart_term PRIVATE -Wall -Wextra -Wpedantic)

add_executable(hal_uart_echo
    uart_echo.c
)

target_link_libraries(hal_uart_echo PRIVATE emberlite_hal)
target_compile_options(hal_uart_echo PRIVATE -Wall -Wextra -Wpedantic)

