# Copyright (C) 2025 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(odr-violation VERSION 0.1 LANGUAGES CXX)
find_package(Qt6 REQUIRED COMPONENTS Qml)
qt_standard_project_setup(REQUIRES 6.8)

qt_add_qml_module(odr-violation-test
    STATIC
    URI OdrViolationTest
    SOURCES
        MyItem.h
    DEPENDENCIES
        QtQml
    OUTPUT_DIRECTORY
        ${CMAKE_CURRENT_BINARY_DIR}/OdrViolationTest
)

target_link_libraries(odr-violation-test
    PRIVATE
        Qt6::Qml
        odr-violation-testplugin
)

qt_add_library(odr-violation-test-app STATIC main.cpp)
target_link_libraries(odr-violation-test-app
    PRIVATE
        odr-violation-test
)
