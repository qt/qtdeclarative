# Copyright (C) 2025 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(flatstyle LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick QuickControls2 Widgets)

qt_standard_project_setup(REQUIRES 6.8)

add_subdirectory(imports/Theme)
add_subdirectory(imports/Flat)

qt_add_executable(flatstyleexample WIN32 MACOSX_BUNDLE
    main.cpp
)

qt_add_qml_module(flatstyleexample
    URI FlatStyleApp
    NO_RESOURCE_TARGET_PATH
    QML_FILES
        "MainForm.ui.qml"
        "flatstyle.qml"
    RESOURCES
        "qtquickcontrols2.conf"
    DEPENDENCIES
        TARGET Flat
        TARGET FlatTheme
)

target_link_libraries(flatstyleexample PUBLIC
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::QuickControls2
    Qt6::Widgets
    Flatplugin
    FlatThemeplugin
)

install(TARGETS flatstyleexample
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_qml_app_script(
    TARGET flatstyleexample
    OUTPUT_SCRIPT deploy_script
    NO_UNSUPPORTED_PLATFORM_ERROR
    DEPLOY_USER_QML_MODULES_ON_UNSUPPORTED_PLATFORM
)
install(SCRIPT ${deploy_script})
