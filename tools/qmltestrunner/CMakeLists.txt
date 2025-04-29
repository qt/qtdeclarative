# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from qmltestrunner.pro.

#####################################################################
## qmltestrunner Tool:
#####################################################################

qt_internal_add_app(qmltestrunner
    TARGET_DESCRIPTION "QML Test Runner"
    INSTALL_DIR "${INSTALL_LIBEXECDIR}"
    SOURCES
        main.cpp
    LIBRARIES
        Qt::Gui
        Qt::Qml
        Qt::QuickTest
)

set_target_properties(qmltestrunner PROPERTIES WIN32_EXECUTABLE FALSE)

if(MACOS AND NOT CMAKE_GENERATOR STREQUAL "Xcode")
    # Add com.apple.security.get-task-allow entitlement to runner,
    # so we can hook into the Swift crash handling.
    set(entitlements_file
        "${__qt_internal_cmake_apple_support_files_path}/test.entitlements.plist")
    add_custom_command(TARGET qmltestrunner
        POST_BUILD COMMAND codesign --sign - -f
            --entitlements "${entitlements_file}"
            "$<TARGET_FILE:qmltestrunner>"

        )
endif()

