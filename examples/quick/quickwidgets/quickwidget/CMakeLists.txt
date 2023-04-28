# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(quickwidget LANGUAGES CXX)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick/quickwidgets/quickwidget")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick QuickWidgets Widgets)

qt_standard_project_setup(REQUIRES 6.5)

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
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
