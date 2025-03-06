# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Create a header containing a hash that describes this library.  For a
# released version of Qt, we'll use the .tag file that is updated by git
# archive with the tree hash. For unreleased versions, we'll do
# "git show -s --format=format:%T" to get the tree hash. If none of this
# works, we use CMake to hash all the files in the src/qml/ directory.
# Skip recreation of the hash when doing a developer build.
function(qt_declarative_write_tag_header target_name)
    set(out_file "${CMAKE_CURRENT_BINARY_DIR}/qml_compile_hash_p.h")
    if(FEATURE_developer_build AND EXISTS "${out_file}")
        target_sources(${target_name} PRIVATE "${out_file}")
        set_source_files_properties("${out_file}" PROPERTIES GENERATED TRUE)
        return()
    endif()

    set(tag_file "${CMAKE_CURRENT_SOURCE_DIR}/../../.tag")
    set(tag_contents "")
    if(EXISTS "${tag_file}")
        file(READ "${tag_file}" tag_contents)
        string(STRIP "${tag_contents}" tag_contents)
    endif()
    find_program(git_path git)

    if(tag_contents AND NOT tag_contents STREQUAL "$Format:%T$")
        set(QML_COMPILE_HASH "${tag_contents}")
    elseif(git_path AND EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../.git")
        execute_process(
            COMMAND ${git_path} show -s --format=format:%T HEAD
            OUTPUT_VARIABLE QML_COMPILE_HASH
            OUTPUT_STRIP_TRAILING_WHITESPACE
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}")
    endif()
    string(LENGTH "${QML_COMPILE_HASH}" QML_COMPILE_HASH_LENGTH)
    if(QML_COMPILE_HASH_LENGTH EQUAL 0)
        set(sources_hash "")
        file(GLOB_RECURSE qtqml_source_files "${CMAKE_CURRENT_SOURCE_DIR}/*")
        foreach(file IN LISTS qtqml_source_files)
            file(SHA1 ${file} file_hash)
            string(APPEND sources_hash ${file_hash})
        endforeach()
        string(SHA1 QML_COMPILE_HASH "${sources_hash}")
    endif()

    string(LENGTH "${QML_COMPILE_HASH}" QML_COMPILE_HASH_LENGTH)
    if(QML_COMPILE_HASH_LENGTH GREATER 0)
        configure_file("qml_compile_hash_p.h.in" "${out_file}")
    else()
        message(FATAL_ERROR "QML compile hash is empty! "
                            "You need either a valid git repository or a non-empty .tag file.")
    endif()
    target_sources(${target_name} PRIVATE "${out_file}")
    set_source_files_properties("${out_file}" PROPERTIES GENERATED TRUE)
endfunction()

# Generate a header file containing a regular expression jit table.
function(qt_declarative_generate_reg_exp_jit_tables consuming_target)
    set(generate_dir "${CMAKE_CURRENT_BINARY_DIR}/.generated")
    if(CMAKE_BUILD_TYPE STREQUAL "Debug")
        string(APPEND generate_dir "/debug")
    elseif(CMAKE_BUILD_TYPE STREQUAL "Release")
        string(APPEND generate_dir "/release")
    endif()

    set(output_file "${generate_dir}/RegExpJitTables.h")
    set(retgen_script_file "${CMAKE_CURRENT_SOURCE_DIR}/../3rdparty/masm/yarr/create_regex_tables")

    add_custom_command(
        OUTPUT "${output_file}"
        COMMAND "${QT_INTERNAL_DECLARATIVE_PYTHON}" ${retgen_script_file} ${output_file}
        MAIN_DEPENDENCY ${retgen_script_file}
        VERBATIM
    )
    target_sources(${consuming_target} PRIVATE ${output_file})
    target_include_directories(${consuming_target} PRIVATE $<BUILD_INTERFACE:${generate_dir}>)
    set_source_files_properties(${output_file} PROPERTIES
        GENERATED TRUE
        _qt_non_module_header TRUE)
endfunction()
