# Generates a header file with a hash.
function(qt_declarative_write_tag_header target_name)
    set(tag_file "${CMAKE_CURRENT_SOURCE_DIR}/../../.tag")
    set(tag_contents "")
    if(EXISTS "${tag_file}")
        file(READ "${tag_file}" tag_contents)
    endif()
    if(NOT tag_file STREQUAL "$Format:%H$")
        set(QML_COMPILE_HASH "${tag_contents}")
        string(STRIP "${QML_COMPILE_HASH}" QML_COMPILE_HASH)
    elseif(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/../../.git")
        execute_process(
            COMMAND git rev-parse HEAD
            OUTPUT_VARIABLE QML_COMPILE_HASH
            OUTPUT_STRIP_TRAILING_WHITESPACE)
    endif()
    string(LENGTH "${QML_COMPILE_HASH}" QML_COMPILE_HASH_LENGTH)
    set(compile_hash_contents "// Generated file, DO NOT EDIT
        #define QML_COMPILE_HASH \"${QML_COMPILE_HASH}\"
        #define QML_COMPILE_HASH_LENGTH ${QML_COMPILE_HASH_LENGTH}"
        )
    set(compile_hash_output_path "${CMAKE_CURRENT_BINARY_DIR}/qml_compile_hash_p.h")
    file(GENERATE OUTPUT "${compile_hash_output_path}"
         CONTENT "${compile_hash_contents}")
endfunction()

find_package(PythonInterp REQUIRED)

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
        COMMAND "${PYTHON_EXECUTABLE}" ${retgen_script_file} ${output_file}
        MAIN_DEPENDENCY ${retgen_script_file}
    )
    target_sources(${consuming_target} PRIVATE ${output_file})
    target_include_directories(${consuming_target} PRIVATE $<BUILD_INTERFACE:${generate_dir}>)
endfunction()
