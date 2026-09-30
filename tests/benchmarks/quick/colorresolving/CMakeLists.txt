# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from colorresolving.pro.

#####################################################################
## tst_colorresolving Test:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_colorresolving LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_benchmark(tst_colorresolving
    SOURCES
        tst_colorresolving.cpp
    LIBRARIES
        Qt::Gui
        Qt::QuickTest
)

qt_internal_extend_target(tst_colorresolving CONDITION NOT ANDROID AND NOT IOS
DEFINES
    QUICK_TEST_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}/data"
)

qt_internal_extend_target(tst_colorresolving CONDITION ANDROID OR IOS
DEFINES
    QUICK_TEST_SOURCE_DIR=":/data"
)
