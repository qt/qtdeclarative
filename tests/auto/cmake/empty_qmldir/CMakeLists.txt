# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(empty_qmldir_test)

find_package(Qt6 ${CMAKE_Core_MODULE_MAJOR_VERSION}.${CMAKE_Core_MODULE_MINOR_VERSION}
    REQUIRED COMPONENTS Qml Gui Test
)

set(CMAKE_AUTOMOC ON)

# Simplify finding the backing targets' DLLs on Windows
if(WIN32)
    set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
endif()

qt_add_executable(empty_qmldir_test main.cpp)
target_link_libraries(empty_qmldir_test PRIVATE Qt6::Test)
if(ANDROID)
    # Tests link to Gui by default beacuse it's needed for Android (QTBUG-83997) when
    # using qt_internal_add_test() but not here where qt_add_executable() is used.
    target_link_libraries(empty_qmldir_test PRIVATE Qt6::Gui)
endif()

qt_add_qml_module(empty_qmldir_test
    URI Third
    VERSION 1.0
    NO_RESOURCE_TARGET_PATH
    SOURCES
        enemy.cpp
        enemy.h
    QML_FILES
        flee.qml
)

# Ensure linting runs when building the default "all" target
set_target_properties(all_qmllint PROPERTIES EXCLUDE_FROM_ALL FALSE)
