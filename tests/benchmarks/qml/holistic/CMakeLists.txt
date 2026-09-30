# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from holistic.pro.

#####################################################################
## tst_holistic Binary:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_holistic LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_benchmark(tst_holistic
    SOURCES
        testtypes.cpp testtypes.h
        tst_holistic.cpp
    DEFINES
        SRCDIR="${CMAKE_CURRENT_SOURCE_DIR}"
    LIBRARIES
        Qt::Gui
        Qt::Network
        Qt::Qml
        Qt::Test
)

#### Keys ignored in scope 1:.:.:holistic.pro:<TRUE>:
# TEMPLATE = "app"

## Scopes:
#####################################################################
