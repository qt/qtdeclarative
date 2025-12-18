# Copyright (C) 2025 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(${RunCMake_TEST} LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Qml Core)

qt_standard_project_setup(REQUIRES 6.8)

add_subdirectory(Main)
add_subdirectory(Other)

qt_add_executable(aot_lookup_validation main.cpp)
target_link_libraries(aot_lookup_validation PRIVATE Qt6::Qml)
add_dependencies(aot_lookup_validation main)
add_dependencies(aot_lookup_validation other)
