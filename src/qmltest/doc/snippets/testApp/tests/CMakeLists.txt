# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

cmake_minimum_required(VERSION 3.2)

project(TestMyApplication LANGUAGES CXX)

enable_testing()

find_package(Qt6 REQUIRED COMPONENTS QuickTest Qml)

qt_standard_project_setup(REQUIRES 6.6)

add_definitions(-DQUICK_TEST_SOURCE_DIR="${CMAKE_CURRENT_SOURCE_DIR}")

#! [link against library]
add_executable(TestMyApplication main.cpp
               setup.cpp setup.h)

add_test(NAME TestMyApplication COMMAND TestMyApplication)

target_link_libraries(TestMyApplication
    PRIVATE
        Qt6::QuickTest
        Qt6::Qml
        MyModule
        MyModuleplugin
)
#! [link against library]
