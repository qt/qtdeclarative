# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(qquickwidgetversuswindow_opengl LANGUAGES CXX)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick/quickwidgets/qquickwidgetversuswindow_opengl")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick QuickWidgets ShaderTools Widgets)

qt_standard_project_setup(REQUIRES 6.5)

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
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
