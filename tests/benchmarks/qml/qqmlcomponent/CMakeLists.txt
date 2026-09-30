# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from qqmlcomponent.pro.

#####################################################################
## tst_qqmlcomponent Binary:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qqmlcomponent_benchmark LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_benchmark(tst_qqmlcomponent_benchmark # special case avoid collision with auto test
    GUI
    SOURCES
        testtypes.cpp testtypes.h
        tst_qqmlcomponent.cpp
    DEFINES
        SRCDIR="${CMAKE_CURRENT_SOURCE_DIR}"
    LIBRARIES
        Qt::Gui
        Qt::Qml
        Qt::Test
)

#### Keys ignored in scope 1:.:.:qqmlcomponent.pro:<TRUE>:
# TEMPLATE = "app"

## Scopes:
#####################################################################
