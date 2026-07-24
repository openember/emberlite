add_library(ember_ppool STATIC
    src/ppool.c
    src/ppool_queue.c
    src/ppool_errno.c
)

target_include_directories(ember_ppool
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}/include
)

find_package(Threads REQUIRED)
target_link_libraries(ember_ppool PUBLIC Threads::Threads)

target_compile_options(ember_ppool PRIVATE -Wall -Wextra -Wpedantic)

if(OPENEMBER_ENABLE_EXAMPLES)
    add_executable(ember_ppool_example example.c)
    target_link_libraries(ember_ppool_example PRIVATE ember_ppool)
    target_compile_options(ember_ppool_example PRIVATE -Wall -Wextra -Wpedantic)
endif()
