# Copyright (C) 2025 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

function(qt6_target_qml_from_svg target)
    set(opt_args
        CURVE_RENDERER
        ASYNCHRONOUS_SHAPES
        OPTIMIZE_PATHS
        OUTLINE_STROKE_MODE)
    set(single_args
        TYPE_NAME
        COPYRIGHT_STATEMENT)
    set(multi_args
        FILES
        OUTPUTS)

    cmake_parse_arguments(PARSE_ARGV 1 arg "${opt_args}" "${single_args}" "${multi_args}")
    _qt_internal_validate_all_args_are_parsed(arg)

    if(NOT arg_FILES)
        message(FATAL_ERROR "qt6_target_qml_from_svg: Missing input files.")
    endif()

    if(NOT arg_OUTPUTS)
        message(FATAL_ERROR "qt6_target_qml_from_svg: Missing output files.")
    endif()

    get_target_property(val ${target} _qt_qml_module_uri)
    if("${val}" MATCHES "-NOTFOUND$")
        message(FATAL_ERROR "qt6_target_qml_from_svg: Target '${target}' must be a QML module.")
    endif()

    math(EXPR file_index "0")
    foreach(filepath IN LISTS arg_FILES)
        get_filename_component(filename "${filepath}" NAME_WLE)
        get_filename_component(file_absolute ${filepath} ABSOLUTE)

        set(svgtoqml_args "")
        if (arg_CURVE_RENDERER)
            list(APPEND svgtoqml_args "-c")
        endif()

        if (arg_ASYNCHRONOUS_SHAPES)
            list(APPEND svgtoqml_args "-a")
        endif()

        if (arg_OPTIMIZE_PATHS)
            list(APPEND svgtoqml_args "-p")
        endif()

        if (arg_OUTLINE_STROKE_MODE)
            list(APPEND svgtoqml_args "--outline-stroke-mode")
        endif()

        if(arg_COPYRIGHT_STATEMENT)
            list(APPEND svgtoqml_args "--copyright-statement" "${arg_COPYRIGHT_STATEMENT}")
        endif()

        if (arg_TYPE_NAME)
            list(APPEND svgtoqml_args "-t" "${arg_TYPE_NAME}")
        endif()

        list(GET arg_OUTPUTS ${file_index} output_file)
        set(svgtoqml_result "${CMAKE_CURRENT_BINARY_DIR}/.qt/svgtoqml/${output_file}")

        list(APPEND svgtoqml_args "${file_absolute}")
        list(APPEND svgtoqml_args "${svgtoqml_result}")

        _qt_internal_get_tool_wrapper_script_path(tool_wrapper)
        add_custom_command(
            OUTPUT "${svgtoqml_result}"
            COMMAND
                ${tool_wrapper}
                $<TARGET_FILE:${QT_CMAKE_EXPORT_NAMESPACE}::svgtoqml>
                ${svgtoqml_args}
            DEPENDS
                "${file_absolute}"
                ${QT_CMAKE_EXPORT_NAMESPACE}::svgtoqml
        )

        list(APPEND svgtoqml_files "${svgtoqml_result}")
        set_source_files_properties("${svgtoqml_result}"
                                    PROPERTIES
                                    QT_RESOURCE_ALIAS
                                    "${output_file}")

        math(EXPR file_index "${file_index}+1")

    endforeach()

    qt6_target_qml_sources(
        ${target}
        QML_FILES ${svgtoqml_files}
        NO_LINT
    )
endfunction()

if(NOT QT_NO_CREATE_VERSIONLESS_FUNCTIONS)
    function(qt_target_qml_from_svg)
        qt6_target_qml_from_svg(${ARGV})
    endfunction()
endif()
