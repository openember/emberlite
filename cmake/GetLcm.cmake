# lcm（独立 ExternalProject，避免与其它第三方库 target 名冲突）

include(${CMAKE_SOURCE_DIR}/cmake/ThirdPartyArchive.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/EmberliteThirdPartyExternal.cmake)

function(openember_prepare_lcm_source out_var)
    if(OPENEMBER_LCM_LOCAL_SOURCE)
        set(_src "${OPENEMBER_LCM_LOCAL_SOURCE}")
    else()
        openember_third_party_prepare_stage(_src "${OPENEMBER_LCM_CACHE_KEY}" "${OPENEMBER_LCM_STAGE_DIR_NAME}"
            "${OPENEMBER_LCM_URL}" "CMakeLists.txt" "")
    endif()
    set(${out_var} "${_src}" PARENT_SCOPE)
endfunction()

function(openember_get_lcm)
    openember_prepare_lcm_source(_src)

    set(_ep "emberlite_ext_lcm")
    set(_bindir "${CMAKE_BINARY_DIR}/_deps-build/${OPENEMBER_LCM_STAGE_DIR_NAME}")
    set(_prefix "${CMAKE_BINARY_DIR}/_deps-install/${OPENEMBER_LCM_STAGE_DIR_NAME}")
    set(_artifact "${_prefix}/lib/liblcm.a")

    emberlite_third_party_external_cmake(${_ep} "${_src}" "${_bindir}" "${_prefix}" "${_artifact}"
        -DLCM_ENABLE_EXAMPLES:BOOL=OFF
        -DLCM_ENABLE_TESTS:BOOL=OFF
        -DLCM_ENABLE_PYTHON:BOOL=OFF
        -DLCM_ENABLE_JAVA:BOOL=OFF
        -DLCM_ENABLE_LUA:BOOL=OFF
    )

    add_library(emberlite_thirdparty_lcm_import STATIC IMPORTED GLOBAL)
    add_dependencies(emberlite_thirdparty_lcm_import ${_ep})
    set_target_properties(emberlite_thirdparty_lcm_import PROPERTIES
        IMPORTED_LOCATION "${_artifact}"
        INTERFACE_INCLUDE_DIRECTORIES "${_prefix}/include"
    )

    set(OPENEMBER_LCM_INCLUDE_DIRS "${_prefix}/include" PARENT_SCOPE)
    set(OPENEMBER_LCM_LIBRARIES emberlite_thirdparty_lcm_import PARENT_SCOPE)
endfunction()
