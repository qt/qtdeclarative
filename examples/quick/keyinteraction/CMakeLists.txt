# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(keyinteraction LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick)

qt_standard_project_setup(REQUIRES 6.8)

qt_add_executable(keyinteractionexample WIN32 MACOSX_BUNDLE
    main.cpp
)

target_link_libraries(keyinteractionexample PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Qml
    Qt6::Quick
)

# Resources:
qt_add_qml_module(keyinteractionexample
    URI keyinteraction
    QML_FILES
        "ContextMenu.qml"
        "GridMenu.qml"
        "ListMenu.qml"
        "ListViewDelegate.qml"
        "TabMenu.qml"
        "focus.qml"
        "keyinteraction.qml"
    RESOURCES
        "images/arrow.png"
        "images/qt-logo.png"
)

install(TARGETS keyinteractionexample
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_qml_app_script(
    TARGET keyinteractionexample
    OUTPUT_SCRIPT deploy_script
    MACOS_BUNDLE_POST_BUILD
    NO_UNSUPPORTED_PLATFORM_ERROR
    DEPLOY_USER_QML_MODULES_ON_UNSUPPORTED_PLATFORM
)
install(SCRIPT ${deploy_script})
