add_library(ember_runtime STATIC
    src/fsm.c
    src/node_registry.c
)

target_include_directories(ember_runtime
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/include
)

target_compile_options(ember_runtime PRIVATE -Wall -Wextra -Wpedantic)

if(OPENEMBER_ENABLE_EXAMPLES)
    add_executable(ember_runtime_example example.c)
    target_link_libraries(ember_runtime_example PRIVATE ember_runtime)
    target_compile_options(ember_runtime_example PRIVATE -Wall -Wextra -Wpedantic)
endif()
