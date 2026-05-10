# LibYAML — https://github.com/yaml/libyaml
# 官方 0.2.5 源码归档已带 CMakeLists.txt（静态库默认 BUILD_SHARED_LIBS=OFF）。
# 若必须坚持 Autotools，可用 ExternalProject 的 CONFIGURE_COMMAND/BUILD_COMMAND 单独封装（当前未启用）。

include(${CMAKE_SOURCE_DIR}/cmake/ThirdPartyArchive.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/EmberliteThirdPartyExternal.cmake)

function(openember_prepare_libyaml_source out_var)
    if(OPENEMBER_LIBYAML_LOCAL_SOURCE)
        set(_src "${OPENEMBER_LIBYAML_LOCAL_SOURCE}")
    else()
        openember_third_party_prepare_stage(_src "${OPENEMBER_LIBYAML_CACHE_KEY}" "${OPENEMBER_LIBYAML_STAGE_DIR_NAME}"
            "${OPENEMBER_LIBYAML_URL}" "CMakeLists.txt" "")
    endif()
    set(${out_var} "${_src}" PARENT_SCOPE)
endfunction()

function(openember_get_libyaml)
    openember_prepare_libyaml_source(_src)

    set(_ep "emberlite_ext_libyaml")
    set(_bindir "${CMAKE_BINARY_DIR}/_deps-build/${OPENEMBER_LIBYAML_STAGE_DIR_NAME}")
    set(_prefix "${CMAKE_BINARY_DIR}/_deps-install/${OPENEMBER_LIBYAML_STAGE_DIR_NAME}")
    set(_artifact "${_prefix}/lib/libyaml.a")

    emberlite_third_party_external_cmake(${_ep} "${_src}" "${_bindir}" "${_prefix}" "${_artifact}"
        -DBUILD_SHARED_LIBS:BOOL=OFF
    )

    add_library(emberlite_thirdparty_libyaml_import STATIC IMPORTED GLOBAL)
    add_dependencies(emberlite_thirdparty_libyaml_import ${_ep})
    set_target_properties(emberlite_thirdparty_libyaml_import PROPERTIES
        IMPORTED_LOCATION "${_artifact}"
        INTERFACE_INCLUDE_DIRECTORIES "${_prefix}/include"
    )

    set(OPENEMBER_LIBYAML_INCLUDE_DIRS "${_prefix}/include" PARENT_SCOPE)
    set(OPENEMBER_LIBYAML_LIBRARIES emberlite_thirdparty_libyaml_import PARENT_SCOPE)
endfunction()
