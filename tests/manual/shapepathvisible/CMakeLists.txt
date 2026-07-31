# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if (NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(shapepathvisible LANGUAGES C CXX)
    find_package(Qt6BuildInternals COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_manual_test(shapepathvisible
    GUI
    SOURCES
        main.cpp
    LIBRARIES
        Qt::Gui
        Qt::Qml
        Qt::Quick
        Qt::QuickControls2
)

# Resources:
set(shapepathvisible_resource_files
    "main.qml"
)

qt_internal_add_resource(shapepathvisible "shapepathvisible"
    PREFIX
        "/shapepathvisible"
    FILES
        ${shapepathvisible_resource_files}
)
