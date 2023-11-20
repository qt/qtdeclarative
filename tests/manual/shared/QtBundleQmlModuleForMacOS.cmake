# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

function(add_qml_module_to_macos_app_bundle app_target qml_plugin_target qml_module_uri)
    if(QT6_IS_SHARED_LIBS_BUILD AND APPLE)
        # The application's main.cpp adds an explicit QML import path to look for qml module plugins
        # under a PlugIns subdirectory of a macOS app bundle.
        # Copy the qmldir and shared library qml plugin.

        # Ensure the executable depends on the plugin so the plugin is copied
        # only after it was built.
        add_dependencies(${app_target} ${qml_plugin_target})

        set(app_dir "$<TARGET_FILE_DIR:${app_target}>")

        string(REGEX REPLACE "[^A-Za-z0-9]" "_" escaped_uri "${qml_module_uri}")

        set(dest_module_dir_in_app_bundle "${app_dir}/../PlugIns/${escaped_uri}")

        set(qml_plugin_dir "$<TARGET_FILE_DIR:${qml_plugin_target}>")
        set(qmldir_file "${qml_plugin_dir}/qmldir")

        add_custom_command(TARGET ${app_target} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E make_directory ${dest_module_dir_in_app_bundle}
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    $<TARGET_FILE:${qml_plugin_target}> ${dest_module_dir_in_app_bundle}
            COMMAND ${CMAKE_COMMAND} -E copy_if_different
                    ${qmldir_file} ${dest_module_dir_in_app_bundle}
        )
    endif()
endfunction()

