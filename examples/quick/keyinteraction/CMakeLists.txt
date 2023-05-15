# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(keyinteraction LANGUAGES CXX)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quick/keyinteraction")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick)

qt_standard_project_setup(REQUIRES 6.5)

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
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
