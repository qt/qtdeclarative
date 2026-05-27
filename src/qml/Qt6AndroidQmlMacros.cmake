# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

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

    # Primitive QML root path: The target's source directory.
    # We need this for backwards compatibility because people might not declare a proper QML module
    # and instead add the .qml files as resources. In that case we won't see them below.
    file(TO_CMAKE_PATH "${target_source_dir}" native_target_source_dir)
    set_property(TARGET ${target} APPEND PROPERTY
        _qt_android_native_qml_root_paths "${native_target_source_dir}")

    # QML root paths, recursively across all linked libraries
    set(root_paths ${target_source_dir})
    _qt_internal_find_qml_root_paths(${target} root_paths)

    # Allow passing extra root paths, for cases when the qml sources might be outside the target
    # source directory, which is the case for some QuickControls tests.
    # Check for both target properties and directory variables.
    get_target_property(extra_root_paths "${target}" QT_QML_IMPORT_SCANNER_EXTRA_ROOT_PATHS)
    if(extra_root_paths)
        list(APPEND root_paths ${extra_root_paths})
    endif()
    if(QT_QML_IMPORT_SCANNER_EXTRA_ROOT_PATHS)
        list(APPEND root_paths ${QT_QML_IMPORT_SCANNER_EXTRA_ROOT_PATHS})
    endif()

    foreach(root_path IN LISTS root_paths)
        file(TO_CMAKE_PATH "${root_path}" native_root_path)
        set_property(TARGET ${target} APPEND PROPERTY
            _qt_android_native_qml_root_paths "${native_root_path}")
    endforeach()

    _qt_internal_add_android_deployment_list_property(${out_var} "qml-root-path"
        ${target} "_qt_android_native_qml_root_paths")

    # Override qmlimportscanner binary path
    _qt_internal_add_tool_to_android_deployment_settings(${out_var} qmlimportscanner
        "qml-importscanner-binary" ${target})

    # Add qml-dom-binary binary path
    _qt_internal_add_tool_to_android_deployment_settings(${out_var} qmldom "qml-dom-binary"
        "${target}")


    _qt_internal_add_android_deployment_list_property(${out_var} "qml-files-for-code-generator"
            ${target} "_qt_qml_files_for_java_generator")

    set(${out_var} "${${out_var}}" PARENT_SCOPE)
endfunction()
