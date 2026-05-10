# EmberLite 最小第三方骨架：模式 + 缓存目录约定；具体 Get*.cmake 在后续按需加入。

if(NOT DEFINED OPENEMBER_THIRD_PARTY_MODE)
    set(OPENEMBER_THIRD_PARTY_MODE "FETCH" CACHE STRING
        "Third-party source mode: FETCH / VENDOR / SYSTEM")
    set_property(CACHE OPENEMBER_THIRD_PARTY_MODE PROPERTY STRINGS FETCH VENDOR SYSTEM)
endif()

include(${CMAKE_SOURCE_DIR}/cmake/ThirdPartyArchive.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/OpenEmberThirdPartyBundleDefaults.cmake)

# -----------------------------------------------------------------------------
# Version pins (唯一来源；升级只改这里并重新配置)
# -----------------------------------------------------------------------------

set(OPENEMBER_NNG_VERSION "1.11" CACHE STRING "nng version/tag")
set(OPENEMBER_NNG_URL
    "https://github.com/nanomsg/nng/archive/refs/tags/v${OPENEMBER_NNG_VERSION}.tar.gz"
    CACHE STRING "nng source archive URL")

set(OPENEMBER_LCM_VERSION "1.5.2" CACHE STRING "lcm version/tag")
set(OPENEMBER_LCM_URL
    "https://github.com/lcm-proj/lcm/archive/refs/tags/v${OPENEMBER_LCM_VERSION}.tar.gz"
    CACHE STRING "lcm source archive URL")

set(OPENEMBER_ZENOHPICO_VERSION "1.9.0" CACHE STRING "zenoh-pico version/tag")
set(OPENEMBER_ZENOHPICO_URL
    "https://github.com/eclipse-zenoh/zenoh-pico/archive/refs/tags/${OPENEMBER_ZENOHPICO_VERSION}.tar.gz"
    CACHE STRING "zenoh-pico source archive URL")

# 解压目录与 third_party 缓存文件名（与上游归档顶层目录一致）
set(OPENEMBER_NNG_CACHE_KEY "nng-${OPENEMBER_NNG_VERSION}")
set(OPENEMBER_NNG_STAGE_DIR_NAME "${OPENEMBER_NNG_CACHE_KEY}")
set(OPENEMBER_LCM_CACHE_KEY "lcm-${OPENEMBER_LCM_VERSION}")
set(OPENEMBER_LCM_STAGE_DIR_NAME "${OPENEMBER_LCM_CACHE_KEY}")
set(OPENEMBER_ZENOHPICO_CACHE_KEY "zenoh-pico-${OPENEMBER_ZENOHPICO_VERSION}")
set(OPENEMBER_ZENOHPICO_STAGE_DIR_NAME "${OPENEMBER_ZENOHPICO_CACHE_KEY}")

# 本地源码覆盖（FETCH/VENDOR 下可用）
set(OPENEMBER_NNG_LOCAL_SOURCE "" CACHE PATH "Optional: pre-extracted nng tree")
set(OPENEMBER_LCM_LOCAL_SOURCE "" CACHE PATH "Optional: pre-extracted lcm tree")
set(OPENEMBER_ZENOHPICO_LOCAL_SOURCE "" CACHE PATH "Optional: pre-extracted zenoh-pico tree")

include(${CMAKE_SOURCE_DIR}/cmake/GetNng.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/GetLcm.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/GetZenohPico.cmake)

################################################################################
# Resolve helpers (FetchContent / Vendor / System)
################################################################################

function(openember_transport_resolve_nng)
    if(OPENEMBER_THIRD_PARTY_MODE STREQUAL "SYSTEM")
        find_path(OPENEMBER_NNG_INCLUDE_DIRS NAMES nng/nng.h)
        find_library(_nng_lib NAMES nng)
        if(NOT OPENEMBER_NNG_INCLUDE_DIRS OR NOT _nng_lib)
            message(FATAL_ERROR "nng not found but OPENEMBER_THIRD_PARTY_MODE=SYSTEM")
        endif()
        set(OPENEMBER_NNG_LIBRARIES ${_nng_lib} PARENT_SCOPE)
        set(OPENEMBER_NNG_INCLUDE_DIRS ${OPENEMBER_NNG_INCLUDE_DIRS} PARENT_SCOPE)
        return()
    endif()

    if(OPENEMBER_THIRD_PARTY_MODE STREQUAL "FETCH" OR OPENEMBER_THIRD_PARTY_MODE STREQUAL "VENDOR")
        if(NOT OPENEMBER_THIRD_PARTY_BUNDLE_NNG)
            message(FATAL_ERROR
                "OPENEMBER_THIRD_PARTY_BUNDLE_NNG=OFF: install libnng-dev or enable bundle (Third-party).")
        endif()
    endif()

    openember_get_nng()
    set(OPENEMBER_NNG_LIBRARIES ${OPENEMBER_NNG_LIBRARIES} PARENT_SCOPE)
    set(OPENEMBER_NNG_INCLUDE_DIRS ${OPENEMBER_NNG_INCLUDE_DIRS} PARENT_SCOPE)
endfunction()

function(openember_transport_resolve_lcm)
    if(OPENEMBER_THIRD_PARTY_MODE STREQUAL "SYSTEM")
        find_path(OPENEMBER_LCM_INCLUDE_DIRS NAMES lcm/lcm.h)
        find_library(_lcm_lib NAMES lcm-static lcm)
        if(NOT OPENEMBER_LCM_INCLUDE_DIRS OR NOT _lcm_lib)
            message(FATAL_ERROR "lcm not found but OPENEMBER_THIRD_PARTY_MODE=SYSTEM")
        endif()
        set(OPENEMBER_LCM_LIBRARIES ${_lcm_lib} PARENT_SCOPE)
        set(OPENEMBER_LCM_INCLUDE_DIRS ${OPENEMBER_LCM_INCLUDE_DIRS} PARENT_SCOPE)
        return()
    endif()

    if(OPENEMBER_THIRD_PARTY_MODE STREQUAL "FETCH" OR OPENEMBER_THIRD_PARTY_MODE STREQUAL "VENDOR")
        if(NOT OPENEMBER_THIRD_PARTY_BUNDLE_LCM)
            message(FATAL_ERROR
                "OPENEMBER_THIRD_PARTY_BUNDLE_LCM=OFF: install liblcm-dev or enable bundle (Third-party).")
        endif()
    endif()

    openember_get_lcm()
    set(OPENEMBER_LCM_LIBRARIES ${OPENEMBER_LCM_LIBRARIES} PARENT_SCOPE)
    set(OPENEMBER_LCM_INCLUDE_DIRS ${OPENEMBER_LCM_INCLUDE_DIRS} PARENT_SCOPE)
endfunction()

function(openember_transport_resolve_zenoh_pico)
    if(OPENEMBER_THIRD_PARTY_MODE STREQUAL "SYSTEM")
        find_path(OPENEMBER_ZENOHPICO_INCLUDE_DIRS NAMES zenoh-pico.h)
        find_library(_zenohpico_lib NAMES zenohpico)
        if(NOT OPENEMBER_ZENOHPICO_INCLUDE_DIRS OR NOT _zenohpico_lib)
            message(FATAL_ERROR "zenoh-pico not found but OPENEMBER_THIRD_PARTY_MODE=SYSTEM")
        endif()
        set(OPENEMBER_ZENOHPICO_LIBRARIES ${_zenohpico_lib} PARENT_SCOPE)
        set(OPENEMBER_ZENOHPICO_INCLUDE_DIRS ${OPENEMBER_ZENOHPICO_INCLUDE_DIRS} PARENT_SCOPE)
        return()
    endif()

    if(OPENEMBER_THIRD_PARTY_MODE STREQUAL "FETCH" OR OPENEMBER_THIRD_PARTY_MODE STREQUAL "VENDOR")
        if(NOT OPENEMBER_THIRD_PARTY_BUNDLE_ZENOHPICO)
            message(FATAL_ERROR
                "OPENEMBER_THIRD_PARTY_BUNDLE_ZENOHPICO=OFF: install libzenohpico-dev or enable bundle (Third-party).")
        endif()
    endif()

    openember_get_zenoh_pico()
    set(OPENEMBER_ZENOHPICO_LIBRARIES ${OPENEMBER_ZENOHPICO_LIBRARIES} PARENT_SCOPE)
    set(OPENEMBER_ZENOHPICO_INCLUDE_DIRS ${OPENEMBER_ZENOHPICO_INCLUDE_DIRS} PARENT_SCOPE)
endfunction()
