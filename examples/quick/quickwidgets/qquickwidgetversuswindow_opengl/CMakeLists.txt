# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(qquickwidgetversuswindow_opengl LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick QuickWidgets ShaderTools Widgets)

qt_standard_project_setup(REQUIRES 6.8)

qt_add_executable(qquickwidgetversuswindow_opengl
    WIN32
    MACOSX_BUNDLE
    fbitem.cpp fbitem.h
    logo.cpp logo.h
    main.cpp
    mainwindow.cpp mainwindow.h
)

target_link_libraries(qquickwidgetversuswindow_opengl PRIVATE
    Qt6::Core
    Qt6::Gui
    Qt6::Quick
    Qt6::QuickWidgets
    Qt6::Widgets
)

qt_add_qml_module(qquickwidgetversuswindow_opengl
    URI fbitem
    QML_FILES test.qml
    RESOURCE_PREFIX /qquickwidgetversuswindow_opengl
    NO_RESOURCE_TARGET_PATH
    DEPENDENCIES QtQuick
)

qt6_add_shaders(qquickwidgetversuswindow_opengl "shaders"
    PRECOMPILE
    OPTIMIZED
    PREFIX
        "/qquickwidgetversuswindow_opengl"
    FILES
        "wobble.frag"
)

install(TARGETS qquickwidgetversuswindow_opengl
    BUNDLE  DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

qt_generate_deploy_qml_app_script(
    TARGET qquickwidgetversuswindow_opengl
    OUTPUT_SCRIPT deploy_script
    MACOS_BUNDLE_POST_BUILD
    NO_UNSUPPORTED_PLATFORM_ERROR
    DEPLOY_USER_QML_MODULES_ON_UNSUPPORTED_PLATFORM
)
install(SCRIPT ${deploy_script})
