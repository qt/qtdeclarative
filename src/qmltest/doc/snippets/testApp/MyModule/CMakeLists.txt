# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

cmake_minimum_required(VERSION 3.20)

project(${MODULE_NAME} LANGUAGES CXX)

qt_standard_project_setup(REQUIRES 6.6)

find_package(Qt6 REQUIRED COMPONENTS Core Gui Qml Quick)

#! [add library]
qt_add_library(MyModule STATIC)

qt6_add_qml_module(MyModule
    URI MyModule
    QML_FILES MyButton.qml
)
#! [add library]

set_target_properties(MyModule PROPERTIES
    MACOSX_BUNDLE TRUE
)

target_link_libraries(MyModule PRIVATE
    Qt::Core
    Qt::Gui
    Qt::Qml
    Qt::Quick
)

target_include_directories(MyModule PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
