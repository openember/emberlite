# EmberLite 最小第三方骨架：模式 + 缓存目录约定；具体 Get*.cmake 在后续按需加入。

if(NOT DEFINED OPENEMBER_THIRD_PARTY_MODE)
    set(OPENEMBER_THIRD_PARTY_MODE "FETCH" CACHE STRING
        "Third-party source mode: FETCH / VENDOR / SYSTEM")
    set_property(CACHE OPENEMBER_THIRD_PARTY_MODE PROPERTY STRINGS FETCH VENDOR SYSTEM)
endif()

include(${CMAKE_SOURCE_DIR}/cmake/ThirdPartyArchive.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/OpenEmberThirdPartyBundleDefaults.cmake)
