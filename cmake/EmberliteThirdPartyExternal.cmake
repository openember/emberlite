# 将第三方库放到独立 binary/install 前缀下构建，避免与主工程或其它第三方库的
# 全局 CMake target 名称冲突（例如 nng 与 lcm 都定义了名为 dist 的 custom target）。

include(ExternalProject)

function(emberlite_third_party_external_cmake name source_dir binary_dir install_prefix byproducts)
    ExternalProject_Add(${name}
        SOURCE_DIR "${source_dir}"
        BINARY_DIR "${binary_dir}"
        INSTALL_DIR "${install_prefix}"
        CMAKE_ARGS
        -DCMAKE_INSTALL_PREFIX:PATH=${install_prefix}
        -DCMAKE_BUILD_TYPE:STRING=${CMAKE_BUILD_TYPE}
        ${ARGN}
        BUILD_BYPRODUCTS ${byproducts}
    )
endfunction()
