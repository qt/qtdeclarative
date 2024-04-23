# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Recursively scans the potential_qml_modules given as list of targets by LINK_LIBRARIES. Examines
# .qml files given as part of the QT_QML_MODULE_QML_FILES target property, and collects their
# directories in out_var. These directories can be used as "root path" for qmlimportscanner.
function(_qt_internal_find_qml_root_paths potential_qml_modules out_var)
    set(qml_root_paths "")
    set(processed "")

    set(potential_qml_modules_queue ${potential_qml_modules})
    while(TRUE)
        list(LENGTH potential_qml_modules_queue length)
        if(${length} STREQUAL "0")
            break()
        endif()

        list(POP_FRONT potential_qml_modules_queue lib)

        if(NOT TARGET ${lib})
            continue()
        endif()

        list(FIND processed ${lib} found)
        if(${found} GREATER_EQUAL "0")
            continue()
        endif()

        get_target_property(root_paths ${lib} _qt_internal_qml_root_path)
        if(root_paths)
            foreach(root_path IN LISTS root_paths)
                list(APPEND qml_root_paths "${root_path}")
            endforeach()
        endif()

        get_target_property(qml_files ${lib} QT_QML_MODULE_QML_FILES)

        foreach(qml_file IN LISTS qml_files)
            get_filename_component(extension "${qml_file}" LAST_EXT)
            if(NOT extension STREQUAL ".qml")
                continue()
            endif()

            get_filename_component(dir "${qml_file}" DIRECTORY)
            get_filename_component(absolute_dir "${dir}" ABSOLUTE)
            list(APPEND qml_root_paths "${absolute_dir}")
        endforeach()

        # We have to consider all dependencies here, not only QML modules.
        # Further QML modules may be indirectly linked via an intermediate library that is not
        # a QML module.
        get_target_property(dependencies ${lib} LINK_LIBRARIES)
        foreach(dependency IN LISTS dependencies)
            list(APPEND potential_qml_modules_queue ${dependency})
        endforeach()

        list(APPEND processed ${lib})
    endwhile()

    list(REMOVE_DUPLICATES qml_root_paths)
    set(${out_var} "${qml_root_paths}" PARENT_SCOPE)
endfunction()

# The function collects qml root paths and sets the _qt_internal_qml_root_path property to the
# ${target} based on the provided qml source files. _qt_internal_qml_root_path is used on purpose
# to not trigger the the QTP0002 warning without user intention.
function(_qt_internal_collect_qml_root_paths target)
    get_target_property(qml_root_paths ${target} _qt_internal_qml_root_path)
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

    get_target_property(potential_qml_modules ${target} LINK_LIBRARIES)

    _qt_internal_find_qml_root_paths(${potential_qml_modules} more_paths)
    if(more_paths)
        foreach(path IN LISTS more_paths)
            list(APPEND qml_root_paths ${path})
        endforeach()
    endif()

    list(REMOVE_DUPLICATES qml_root_paths)
    set_target_properties(${target} PROPERTIES _qt_internal_qml_root_path "${qml_root_paths}")
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

    # Primitive QML root path: The target's source directory.
    # We need this for backwards compatibility because people might not declare a proper QML module
    # and instead add the .qml files as resources. In that case we won't see them below.
    file(TO_CMAKE_PATH "${target_source_dir}" native_target_source_dir)
    set_property(TARGET ${target} APPEND PROPERTY
        _qt_android_native_qml_root_paths "${native_target_source_dir}")

    # QML root paths, recursively across all linked libraries
    set(root_paths ${target_source_dir})
    _qt_internal_find_qml_root_paths(${target} root_paths)
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

    set(${out_var} "${${out_var}}" PARENT_SCOPE)
endfunction()
