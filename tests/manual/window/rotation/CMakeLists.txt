# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(windowrotation VERSION 0.1 LANGUAGES CXX)

find_package(Qt6 COMPONENTS Core Gui Quick QuickControls2 REQUIRED)

qt_standard_project_setup(REQUIRES 6.5)

qt_add_executable(windowrotation
    MANUAL_FINALIZATION
    main.cpp
)

target_link_libraries(windowrotation PRIVATE
    Qt6::Quick Qt6::QuickControls2
)

qt_finalize_executable(windowrotation)

qt_add_qml_module(windowrotation
    URI Main
    VERSION 1.0
    QML_FILES "Main.qml"
)

include_directories(${CMAKE_CURRENT_SOURCE_DIR})
