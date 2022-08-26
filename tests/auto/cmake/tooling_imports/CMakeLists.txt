# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause


cmake_minimum_required(VERSION 3.16)
project(cmake_test)

find_package(Qt6 ${CMAKE_Core_MODULE_MAJOR_VERSION}.${CMAKE_Core_MODULE_MINOR_VERSION}
    REQUIRED COMPONENTS Qml Gui Test
)

set(CMAKE_AUTOMOC ON)

# Simplify finding the backing targets' DLLs on Windows
if(WIN32)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
endif()

add_subdirectory(First)
add_subdirectory(Second)

qt_add_executable(cmake_test main.cpp)
target_link_libraries(cmake_test PRIVATE Qt6::Test Qt6::Gui)

qt_add_qml_module(cmake_test
    URI Test
    VERSION 1.0
    NO_RESOURCE_TARGET_PATH
    QML_FILES
        main.qml
        Ttt.qml
)

# Ensure linting runs when building the default "all" target
set_target_properties(all_qmllint PROPERTIES EXCLUDE_FROM_ALL FALSE)
