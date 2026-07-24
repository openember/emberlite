add_library(ember_logging STATIC
    src/log.c
)

target_include_directories(ember_logging
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/include
)

target_compile_options(ember_logging PRIVATE -Wall -Wextra -Wpedantic)
