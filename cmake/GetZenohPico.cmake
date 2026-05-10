# zenoh-pico（独立 ExternalProject，避免与其它第三方库 target 名冲突）

include(${CMAKE_SOURCE_DIR}/cmake/ThirdPartyArchive.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/EmberliteThirdPartyExternal.cmake)

function(openember_prepare_zenoh_pico_source out_var)
    if(OPENEMBER_ZENOHPICO_LOCAL_SOURCE)
        set(_src "${OPENEMBER_ZENOHPICO_LOCAL_SOURCE}")
    else()
        openember_third_party_prepare_stage(_src "${OPENEMBER_ZENOHPICO_CACHE_KEY}" "${OPENEMBER_ZENOHPICO_STAGE_DIR_NAME}"
            "${OPENEMBER_ZENOHPICO_URL}" "CMakeLists.txt" "")
    endif()
    set(${out_var} "${_src}" PARENT_SCOPE)
endfunction()

function(openember_get_zenoh_pico)
    openember_prepare_zenoh_pico_source(_src)

    set(_ep "emberlite_ext_zenoh_pico")
    set(_bindir "${CMAKE_BINARY_DIR}/_deps-build/${OPENEMBER_ZENOHPICO_STAGE_DIR_NAME}")
    set(_prefix "${CMAKE_BINARY_DIR}/_deps-install/${OPENEMBER_ZENOHPICO_STAGE_DIR_NAME}")
    set(_artifact "${_prefix}/lib/libzenohpico.a")

    emberlite_third_party_external_cmake(${_ep} "${_src}" "${_bindir}" "${_prefix}" "${_artifact}"
        -DBUILD_SHARED_LIBS:BOOL=OFF
        -DBUILD_EXAMPLES:BOOL=OFF
        -DBUILD_TESTING:BOOL=OFF
        -DBUILD_TOOLS:BOOL=OFF
        -DBUILD_INTEGRATION:BOOL=OFF
    )

    add_library(emberlite_thirdparty_zenohpico_import STATIC IMPORTED GLOBAL)
    add_dependencies(emberlite_thirdparty_zenohpico_import ${_ep})
    set_target_properties(emberlite_thirdparty_zenohpico_import PROPERTIES
        IMPORTED_LOCATION "${_artifact}"
        INTERFACE_INCLUDE_DIRECTORIES "${_prefix}/include"
    )

    set(OPENEMBER_ZENOHPICO_INCLUDE_DIRS "${_prefix}/include" PARENT_SCOPE)
    set(OPENEMBER_ZENOHPICO_LIBRARIES emberlite_thirdparty_zenohpico_import PARENT_SCOPE)
endfunction()
