# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(quickwidget LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick QuickWidgets Widgets)

qt_standard_project_setup(REQUIRES 6.8)

qt_add_executable(quickwidget
    WIN32
    MACOSX_BUNDLE
    main.cpp
)

target_link_libraries(quickwidget PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::QuickWidgets
    Qt6::Widgets
)

qt_add_qml_module(quickwidget
    URI QuickWidgetExample
    QML_FILES
        rotatingsquare.qml
    RESOURCE_PREFIX /quickwidget
    NO_RESOURCE_TARGET_PATH
)

install(TARGETS quickwidget
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_qml_app_script(
    TARGET quickwidget
    OUTPUT_SCRIPT deploy_script
    MACOS_BUNDLE_POST_BUILD
    NO_UNSUPPORTED_PLATFORM_ERROR
    DEPLOY_USER_QML_MODULES_ON_UNSUPPORTED_PLATFORM
)
install(SCRIPT ${deploy_script})
