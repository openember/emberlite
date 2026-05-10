# yyjson — https://github.com/ibireme/yyjson（独立 ExternalProject）

include(${CMAKE_SOURCE_DIR}/cmake/ThirdPartyArchive.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/EmberliteThirdPartyExternal.cmake)

function(openember_prepare_yyjson_source out_var)
    if(OPENEMBER_YYJSON_LOCAL_SOURCE)
        set(_src "${OPENEMBER_YYJSON_LOCAL_SOURCE}")
    else()
        openember_third_party_prepare_stage(_src "${OPENEMBER_YYJSON_CACHE_KEY}" "${OPENEMBER_YYJSON_STAGE_DIR_NAME}"
            "${OPENEMBER_YYJSON_URL}" "CMakeLists.txt" "")
    endif()
    set(${out_var} "${_src}" PARENT_SCOPE)
endfunction()

function(openember_get_yyjson)
    openember_prepare_yyjson_source(_src)

    set(_ep "emberlite_ext_yyjson")
    set(_bindir "${CMAKE_BINARY_DIR}/_deps-build/${OPENEMBER_YYJSON_STAGE_DIR_NAME}")
    set(_prefix "${CMAKE_BINARY_DIR}/_deps-install/${OPENEMBER_YYJSON_STAGE_DIR_NAME}")
    set(_artifact "${_prefix}/lib/libyyjson.a")

    emberlite_third_party_external_cmake(${_ep} "${_src}" "${_bindir}" "${_prefix}" "${_artifact}"
        -DBUILD_SHARED_LIBS:BOOL=OFF
        -DYYJSON_BUILD_TESTS:BOOL=OFF
        -DYYJSON_BUILD_FUZZER:BOOL=OFF
        -DYYJSON_BUILD_MISC:BOOL=OFF
        -DYYJSON_BUILD_DOC:BOOL=OFF
    )

    add_library(emberlite_thirdparty_yyjson_import STATIC IMPORTED GLOBAL)
    add_dependencies(emberlite_thirdparty_yyjson_import ${_ep})
    set_target_properties(emberlite_thirdparty_yyjson_import PROPERTIES
        IMPORTED_LOCATION "${_artifact}"
        INTERFACE_INCLUDE_DIRECTORIES "${_prefix}/include"
    )

    set(OPENEMBER_YYJSON_INCLUDE_DIRS "${_prefix}/include" PARENT_SCOPE)
    set(OPENEMBER_YYJSON_LIBRARIES emberlite_thirdparty_yyjson_import PARENT_SCOPE)
endfunction()
