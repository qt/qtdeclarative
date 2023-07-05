# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)

project(cmake_test)

find_package(Qt6 ${CMAKE_Core_MODULE_MAJOR_VERSION}.${CMAKE_Core_MODULE_MINOR_VERSION}
    REQUIRED COMPONENTS Qml Quick Test)

set(CMAKE_AUTOMOC ON)

# Simplify finding the backing targets' DLLs on Windows
if(WIN32)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
else()
    add_compile_options (
        -Werror  # Treat all warnings as errors
        -Wshadow # Warn on shadowing
    )
endif()

add_subdirectory(duck/tick)
add_subdirectory(duck/trick)
add_subdirectory(duck/track)

qt_add_executable(cmake_test main.cpp)
target_link_libraries(cmake_test
    PRIVATE Qt6::Quick Qt6::Test Qt6::Gui duck_tickplugin duck_trickplugin duck_trackplugin)

qt_add_qml_module(cmake_test
    URI duck
    VERSION 1.0
    QML_FILES main.qml
    RESOURCE_PREFIX /
)

