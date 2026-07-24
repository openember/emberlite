# openember-msgs (https://github.com/openember/openember-msgs)
#
# EmberLite consumes the shared protocol definitions through Nanopb-generated C.

include(${CMAKE_SOURCE_DIR}/cmake/ThirdPartyArchive.cmake)
include(${CMAKE_SOURCE_DIR}/cmake/GetNanopb.cmake)

function(openember_prepare_openember_msgs_fetch out_var)
    if(OPENEMBER_THIRD_PARTY_MODE STREQUAL "FETCH")
        set(_deps_parent "${CMAKE_BINARY_DIR}/_deps")
        set(_stage "${_deps_parent}/${OPENEMBER_MSGS_STAGE_DIR_NAME}")

        file(MAKE_DIRECTORY "${_deps_parent}")
        openember_third_party_cache_dir(_cache)
        openember_third_party_download_to_cache_key(
            "${OPENEMBER_MSGS_URL}" "${_cache}" "${OPENEMBER_MSGS_CACHE_KEY}" _archive)

        file(REMOVE_RECURSE "${_stage}")
        message(STATUS "Third-party: extracting ${_archive} -> ${_deps_parent}")
        openember_third_party_extract_archive("${_archive}" "${_deps_parent}")
        if(NOT EXISTS "${_stage}/CMakeLists.txt")
            message(FATAL_ERROR
                "After extract, expected ${_stage}/CMakeLists.txt "
                "(check OPENEMBER_MSGS_REF / OPENEMBER_MSGS_URL).")
        endif()

        set(${out_var} "${_stage}" PARENT_SCOPE)
        return()
    endif()

    openember_third_party_prepare_stage(_src "${OPENEMBER_MSGS_CACHE_KEY}" "${OPENEMBER_MSGS_STAGE_DIR_NAME}"
        "${OPENEMBER_MSGS_URL}" "CMakeLists.txt" "")
    set(${out_var} "${_src}" PARENT_SCOPE)
endfunction()

function(openember_prepare_openember_msgs_source out_var)
    if(NOT (OPENEMBER_MSGS_SOURCE STREQUAL "FETCH") AND NOT (OPENEMBER_MSGS_SOURCE STREQUAL "LOCAL"))
        message(FATAL_ERROR "OPENEMBER_MSGS_SOURCE must be FETCH or LOCAL (got '${OPENEMBER_MSGS_SOURCE}')")
    endif()

    if(OPENEMBER_MSGS_SOURCE STREQUAL "LOCAL")
        if(NOT OPENEMBER_MSGS_LOCAL_SOURCE)
            message(FATAL_ERROR
                "OPENEMBER_MSGS_SOURCE=LOCAL requires OPENEMBER_MSGS_LOCAL_SOURCE "
                "to be an absolute path to openember-msgs.")
        endif()
        if(NOT IS_ABSOLUTE "${OPENEMBER_MSGS_LOCAL_SOURCE}")
            message(FATAL_ERROR "OPENEMBER_MSGS_LOCAL_SOURCE must be absolute: ${OPENEMBER_MSGS_LOCAL_SOURCE}")
        endif()
        if(NOT EXISTS "${OPENEMBER_MSGS_LOCAL_SOURCE}/CMakeLists.txt")
            message(FATAL_ERROR "Expected CMakeLists.txt under OPENEMBER_MSGS_LOCAL_SOURCE=${OPENEMBER_MSGS_LOCAL_SOURCE}")
        endif()
        set(_src "${OPENEMBER_MSGS_LOCAL_SOURCE}")
        message(STATUS "openember-msgs: using local checkout ${_src}")
    else()
        openember_prepare_openember_msgs_fetch(_src)
        message(STATUS "openember-msgs: fetched ${OPENEMBER_MSGS_REF} -> ${_src}")
    endif()

    set(${out_var} "${_src}" PARENT_SCOPE)
endfunction()

function(openember_find_python_with_protobuf out_var)
    if(OPENEMBER_PROTOBUF_PYTHON)
        execute_process(
            COMMAND "${OPENEMBER_PROTOBUF_PYTHON}" -c "import google.protobuf"
            RESULT_VARIABLE _protobuf_python_result
            OUTPUT_QUIET
            ERROR_QUIET
        )
        if(_protobuf_python_result EQUAL 0)
            set(${out_var} "${OPENEMBER_PROTOBUF_PYTHON}" PARENT_SCOPE)
            return()
        endif()

        message(FATAL_ERROR
            "OPENEMBER_PROTOBUF_PYTHON=${OPENEMBER_PROTOBUF_PYTHON} cannot import google.protobuf. "
            "Install the Python protobuf package for that interpreter or clear OPENEMBER_PROTOBUF_PYTHON.")
    endif()

    find_package(Python3 COMPONENTS Interpreter REQUIRED)

    set(_candidates "${Python3_EXECUTABLE}")
    find_program(_system_python3 NAMES python3 PATHS /usr/bin /usr/local/bin NO_DEFAULT_PATH)
    if(_system_python3)
        list(APPEND _candidates "${_system_python3}")
    endif()
    find_program(_path_python3 NAMES python3)
    if(_path_python3)
        list(APPEND _candidates "${_path_python3}")
    endif()
    list(REMOVE_DUPLICATES _candidates)

    set(_checked)
    foreach(_python IN LISTS _candidates)
        if(NOT _python)
            continue()
        endif()

        list(APPEND _checked "${_python}")
        execute_process(
            COMMAND "${_python}" -c "import google.protobuf"
            RESULT_VARIABLE _protobuf_python_result
            OUTPUT_QUIET
            ERROR_QUIET
        )
        if(_protobuf_python_result EQUAL 0)
            set(OPENEMBER_PROTOBUF_PYTHON "${_python}" CACHE FILEPATH
                "Python interpreter with google.protobuf for nanopb generation" FORCE)
            set(${out_var} "${_python}" PARENT_SCOPE)
            message(STATUS "openember-msgs: using protobuf Python ${_python}")
            return()
        endif()
    endforeach()

    string(REPLACE ";" ", " _checked_text "${_checked}")
    message(FATAL_ERROR
        "openember-msgs for EmberLite requires a Python interpreter with google.protobuf. "
        "Checked: ${_checked_text}. Install protobuf for Python or set OPENEMBER_PROTOBUF_PYTHON.")
endfunction()

function(openember_get_openember_msgs)
    if(TARGET emberlite::msgs)
        return()
    endif()

    openember_prepare_openember_msgs_source(_msgs_src)
    openember_get_nanopb()

    openember_find_python_with_protobuf(_protobuf_python)
    find_program(OPENEMBER_PROTOC_EXECUTABLE NAMES protoc)
    if(NOT OPENEMBER_PROTOC_EXECUTABLE)
        message(FATAL_ERROR
            "openember-msgs for EmberLite requires protoc on PATH. "
            "Install protobuf-compiler or disable OPENEMBER_ENABLE_MSGS.")
    endif()

    set(_proto_root "${_msgs_src}/proto")
    set(_options "${_msgs_src}/nanopb/openember_msgs.options")
    set(_generated_root "${CMAKE_BINARY_DIR}/generated/openember-msgs")

    set(_proto_files
        "openember/msgs/common/v1/common.proto"
        "openember/msgs/lifecycle/v1/lifecycle.proto"
        "openember/msgs/node/v1/node.proto"
        "openember/msgs/diagnostics/v1/diagnostics.proto"
        "openember/msgs/parameter/v1/parameter.proto"
        "openember/msgs/log/v1/log.proto"
        "openember/msgs/device/v1/device.proto"
        "openember/msgs/runtime/v1/runtime.proto"
    )

    set(_proto_abs_files)
    foreach(_proto IN LISTS _proto_files)
        list(APPEND _proto_abs_files "${_proto_root}/${_proto}")
    endforeach()

    set(_generated_srcs)
    set(_generated_hdrs)
    foreach(_proto IN LISTS _proto_files)
        get_filename_component(_proto_dir "${_proto}" DIRECTORY)
        get_filename_component(_proto_name "${_proto}" NAME_WE)
        set(_out_dir "${_generated_root}/${_proto_dir}")
        set(_out_src "${_out_dir}/${_proto_name}.pb.c")
        set(_out_hdr "${_out_dir}/${_proto_name}.pb.h")

        file(MAKE_DIRECTORY "${_out_dir}")

        add_custom_command(
            OUTPUT "${_out_src}" "${_out_hdr}"
            COMMAND "${_protobuf_python}"
                    "${OPENEMBER_NANOPB_GENERATOR}"
                    --quiet
                    -I "${_proto_root}"
                    -I "${OPENEMBER_NANOPB_SOURCE_DIR}/generator/proto"
                    -f "${_options}"
                    -D "${_generated_root}"
                    "${_proto_root}/${_proto}"
            DEPENDS ${_proto_abs_files} "${_options}" "${OPENEMBER_NANOPB_GENERATOR}"
            COMMENT "Running nanopb generator on ${_proto}"
            VERBATIM
        )

        list(APPEND _generated_srcs "${_out_src}")
        list(APPEND _generated_hdrs "${_out_hdr}")
    endforeach()

    add_library(emberlite_msgs STATIC
        ${_generated_srcs}
        ${_generated_hdrs}
    )
    add_library(emberlite::msgs ALIAS emberlite_msgs)

    target_include_directories(emberlite_msgs
        PUBLIC
            "${_generated_root}"
    )
    target_link_libraries(emberlite_msgs
        PUBLIC
            emberlite::nanopb
    )

    set(OPENEMBER_MSGS_LIBRARIES emberlite::msgs PARENT_SCOPE)
    set(OPENEMBER_MSGS_INCLUDE_DIRS "${_generated_root}" PARENT_SCOPE)
endfunction()
