add_library(ember_crypto STATIC
    src/crc.c
    src/md5.c
    src/sha256.c
)

target_include_directories(ember_crypto
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/include
)

target_compile_options(ember_crypto PRIVATE -Wall -Wextra -Wpedantic)
