add_library(ember_netdev STATIC
    src/netdev.c
)

target_include_directories(ember_netdev
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/include
)

target_link_libraries(ember_netdev
    PUBLIC
        ember_logging
    PRIVATE
        resolv
)

target_compile_options(ember_netdev PRIVATE -Wall -Wextra)
target_compile_definitions(ember_netdev PRIVATE _DEFAULT_SOURCE)
