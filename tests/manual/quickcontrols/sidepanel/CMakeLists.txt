# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(sidepanel LANGUAGES CXX)

set(CMAKE_AUTOMOC ON)

if(NOT DEFINED INSTALL_EXAMPLESDIR)
    set(INSTALL_EXAMPLESDIR "examples")
endif()

set(INSTALL_EXAMPLEDIR "${INSTALL_EXAMPLESDIR}/quickcontrols/sidepanel")

find_package(Qt6 REQUIRED COMPONENTS Core Gui Quick)

qt_add_executable(sidepanelexample WIN32 MACOSX_BUNDLE
    sidepanel.cpp
)

qt_add_qml_module(sidepanelexample
    URI sidepanel
    NO_RESOURCE_TARGET_PATH
    QML_FILES
        "sidepanel.qml"
    RESOURCES
        "images/qt-logo.png"
        "images/qt-logo@2x.png"
        "images/qt-logo@3x.png"
        "images/qt-logo@4x.png"
        "qtquickcontrols2.conf"
        "doc/images/qtquickcontrols-sidepanel-landscape.png"
        "doc/images/qtquickcontrols-sidepanel-portrait.png"
)

target_link_libraries(sidepanelexample PUBLIC
    Qt::Core
    Qt::Gui
    Qt::Quick
)

install(TARGETS sidepanelexample
    RUNTIME DESTINATION "${INSTALL_EXAMPLEDIR}"
    BUNDLE DESTINATION "${INSTALL_EXAMPLEDIR}"
    LIBRARY DESTINATION "${INSTALL_EXAMPLEDIR}"
)
