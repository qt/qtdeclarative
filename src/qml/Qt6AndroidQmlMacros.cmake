# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# The function collects qml root paths and sets the QT_QML_ROOT_PATH property to the ${target}
# based on the provided qml source files.
function(_qt_internal_collect_qml_root_paths target)
    get_target_property(qml_root_paths ${target} QT_QML_ROOT_PATH)
    if(NOT qml_root_paths)
        set(qml_root_paths "")
    endif()
    foreach(file IN LISTS ARGN)
        get_filename_component(extension "${file}" LAST_EXT)
        if(NOT extension STREQUAL ".qml")
            continue()
        endif()

        get_filename_component(dir "${file}" DIRECTORY)
        get_filename_component(absolute_dir "${dir}" ABSOLUTE)
        list(APPEND qml_root_paths "${absolute_dir}")
    endforeach()

    list(REMOVE_DUPLICATES qml_root_paths)
    set_target_properties(${target} PROPERTIES QT_QML_ROOT_PATH "${qml_root_paths}")
endfunction()

# The function extracts the required QML properties from the 'target' and
# appends them to the 'out_var' using the corresponding JSON keys.
function(_qt_internal_generate_android_qml_deployment_settings out_var target)
    get_target_property(target_source_dir ${target} SOURCE_DIR)

    # QML import paths
    _qt_internal_collect_target_qml_import_paths(qml_import_paths ${target})
    get_target_property(native_qml_import_paths "${target}" _qt_native_qml_import_paths)
    if(native_qml_import_paths)
        list(PREPEND native_qml_import_paths "${qml_import_paths}")
    else()
        set(native_qml_import_paths "${qml_import_paths}")
    endif()
    list(REMOVE_DUPLICATES native_qml_import_paths)
    set_property(TARGET "${target}" PROPERTY
        _qt_native_qml_import_paths "${native_qml_import_paths}")
    _qt_internal_add_android_deployment_multi_value_property(${out_var} "qml-import-paths"
        ${target} "_qt_native_qml_import_paths")

    # QML root paths
    file(TO_CMAKE_PATH "${target_source_dir}" native_target_source_dir)
    set_property(TARGET ${target} APPEND PROPERTY
        _qt_android_native_qml_root_paths "${native_target_source_dir}")
    _qt_internal_add_android_deployment_list_property(${out_var} "qml-root-path"
        ${target} "_qt_android_native_qml_root_paths")

    # Override qmlimportscanner binary path
    _qt_internal_add_tool_to_android_deployment_settings(${out_var} qmlimportscanner
        "qml-importscanner-binary" ${target})

    set(${out_var} "${${out_var}}" PARENT_SCOPE)
endfunction()
