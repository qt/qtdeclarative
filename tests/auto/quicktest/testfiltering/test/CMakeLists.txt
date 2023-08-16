# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from test.pro.

#####################################################################
## tst_testfiltering Test:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_testfiltering LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_test(tst_testfiltering
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../"
    WORKING_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../" # special case
    SOURCES
        ../tst_testfiltering.cpp
    LIBRARIES
        Qt::Gui
)
