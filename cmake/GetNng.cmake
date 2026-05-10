# nng（独立 ExternalProject，避免与其它第三方库 target 名冲突）

include(${CMAKE_SOURCE_DIR}/cmake/ThirdPartyArchive.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/EmberliteThirdPartyExternal.cmake)

function(openember_prepare_nng_source out_var)
    if(OPENEMBER_NNG_LOCAL_SOURCE)
        set(_src "${OPENEMBER_NNG_LOCAL_SOURCE}")
    else()
        openember_third_party_prepare_stage(_src "${OPENEMBER_NNG_CACHE_KEY}" "${OPENEMBER_NNG_STAGE_DIR_NAME}"
            "${OPENEMBER_NNG_URL}" "CMakeLists.txt" "")
    endif()
    set(${out_var} "${_src}" PARENT_SCOPE)
endfunction()

function(openember_get_nng)
    openember_prepare_nng_source(_src)

    set(_ep "emberlite_ext_nng")
    set(_bindir "${CMAKE_BINARY_DIR}/_deps-build/${OPENEMBER_NNG_STAGE_DIR_NAME}")
    set(_prefix "${CMAKE_BINARY_DIR}/_deps-install/${OPENEMBER_NNG_STAGE_DIR_NAME}")
    set(_artifact "${_prefix}/lib/libnng.a")

    emberlite_third_party_external_cmake(${_ep} "${_src}" "${_bindir}" "${_prefix}" "${_artifact}"
        -DBUILD_SHARED_LIBS:BOOL=OFF
        -DNNG_TESTS:BOOL=OFF
        -DNNG_TOOLS:BOOL=OFF
        -DNNG_ENABLE_COVERAGE:BOOL=OFF
        -DNNG_ENABLE_STATS:BOOL=OFF
        -DNNG_ENABLE_HTTP:BOOL=OFF
        -DNNG_TRANSPORT_WS:BOOL=OFF
        -DNNG_TRANSPORT_WSS:BOOL=OFF
        -DNNG_ENABLE_NNGCAT:BOOL=OFF
    )

    add_library(emberlite_thirdparty_nng_import STATIC IMPORTED GLOBAL)
    add_dependencies(emberlite_thirdparty_nng_import ${_ep})
    set_target_properties(emberlite_thirdparty_nng_import PROPERTIES
        IMPORTED_LOCATION "${_artifact}"
        INTERFACE_INCLUDE_DIRECTORIES "${_prefix}/include"
    )

    set(OPENEMBER_NNG_INCLUDE_DIRS "${_prefix}/include" PARENT_SCOPE)
    set(OPENEMBER_NNG_LIBRARIES emberlite_thirdparty_nng_import PARENT_SCOPE)
endfunction()
