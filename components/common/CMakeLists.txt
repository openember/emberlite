add_library(ember_common STATIC
    src/common.c
)

target_include_directories(ember_common
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/include
)

target_compile_options(ember_common PRIVATE -Wall -Wextra -Wpedantic)
