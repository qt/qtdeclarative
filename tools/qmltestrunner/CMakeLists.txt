# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from qmltestrunner.pro.

#####################################################################
## qmltestrunner Tool:
#####################################################################

function(qt_internal_add_entitlements_to_qmltestrunner target)
    if(MACOS AND NOT CMAKE_GENERATOR STREQUAL "Xcode")
        # Add com.apple.security.get-task-allow entitlement to runner,
        # so we can hook into the Swift crash handling.
        set(entitlements_file
            "${__qt_internal_cmake_apple_support_files_path}/test.entitlements.plist")
        add_custom_command(TARGET ${target}
            POST_BUILD COMMAND codesign --sign - -f
                --entitlements "${entitlements_file}"
                "$<TARGET_FILE:${target}>"

            )
    endif()
endfunction()

# To temporarily fix QTBUG-137005, always build qmltestrunner for the target arch, even when
# cross-compiling, by locally overriding QT_WILL_BUILD_TOOLS and QT_WILL_RENAME_TOOL_TARGETS to
# ON. This behaves as if one passed -DQT_FORCE_BUILD_TOOLS=ON, but only for this specific tool.
# In a cross-compiling case the qt_internal_add_tool call will
# - find the host tool
# - build the tool with the target arch, as well as install the target tool
# A Qt6QmlToolsConfig.cmake file will NOT be created because QT_WILL_BUILD_TOOLS will
# not be true in the scope of qt_build_repo -> qt_build_repo_post_process ->
# qt_create_tools_config_files -> qt_export_tools calls.
# This is actually desirable for now, until we switch qmltestrunner to be an app instead of a
# tool.
if(CMAKE_CROSSCOMPILING AND NOT QT_NO_TARGET_QMLTESTRUNNER)
    set(QT_WILL_BUILD_TOOLS TRUE)
    set(QT_WILL_RENAME_TOOL_TARGETS TRUE)
endif()

qt_get_tool_target_name(target_name qmltestrunner)

qt_internal_add_tool(${target_name}
    TARGET_DESCRIPTION "QML Test Runner"
    TOOLS_TARGET Qml # special case
    SOURCES
        main.cpp
    LIBRARIES
        Qt::Gui
        Qt::Qml
        Qt::QuickTest
)
qt_internal_return_unless_building_tools()

qt_internal_add_entitlements_to_qmltestrunner(${target_name})
