# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(qmltypememory LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

# Generated from qmltypememory.pro.

#####################################################################
## qmltypememory Binary:
#####################################################################

qt_internal_add_manual_test(qmltypememory
    GUI
    SOURCES
        main.cpp
        testdriver.cpp testdriver.h
    INCLUDE_DIRECTORIES
        .
    LIBRARIES
        Qt::Gui
        Qt::Qml
)

#### Keys ignored in scope 1:.:.:qmltypememory.pro:<TRUE>:
# TEMPLATE = "app"
