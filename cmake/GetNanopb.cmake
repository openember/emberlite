# Nanopb (https://github.com/nanopb/nanopb)
#
# Pure C protobuf runtime and generator used by EmberLite.

include(${CMAKE_SOURCE_DIR}/cmake/ThirdPartyArchive.cmake)

function(openember_prepare_nanopb_source out_var)
    if(OPENEMBER_NANOPB_LOCAL_SOURCE)
        set(_src "${OPENEMBER_NANOPB_LOCAL_SOURCE}")
    else()
        openember_third_party_prepare_stage(_src "${OPENEMBER_NANOPB_CACHE_KEY}" "${OPENEMBER_NANOPB_STAGE_DIR_NAME}"
            "${OPENEMBER_NANOPB_URL}" "generator/nanopb_generator.py" "")
    endif()

    set(${out_var} "${_src}" PARENT_SCOPE)
endfunction()

function(openember_get_nanopb)
    if(TARGET emberlite::nanopb)
        set(OPENEMBER_NANOPB_SOURCE_DIR "${OPENEMBER_NANOPB_SOURCE_DIR}" PARENT_SCOPE)
        set(OPENEMBER_NANOPB_GENERATOR "${OPENEMBER_NANOPB_GENERATOR}" PARENT_SCOPE)
        return()
    endif()

    openember_prepare_nanopb_source(_src)

    add_library(emberlite_nanopb STATIC
        "${_src}/pb_common.c"
        "${_src}/pb_decode.c"
        "${_src}/pb_encode.c"
    )
    add_library(emberlite::nanopb ALIAS emberlite_nanopb)

    target_include_directories(emberlite_nanopb
        PUBLIC
            "${_src}"
    )
    target_compile_definitions(emberlite_nanopb
        PUBLIC
            PB_FIELD_32BIT=1
    )

    set(OPENEMBER_NANOPB_SOURCE_DIR "${_src}" PARENT_SCOPE)
    set(OPENEMBER_NANOPB_GENERATOR "${_src}/generator/nanopb_generator.py" PARENT_SCOPE)
endfunction()
