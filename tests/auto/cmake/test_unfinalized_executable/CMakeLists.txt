# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(test_unfinalized_executable LANGUAGES CXX)

find_package(Qt6 ${CMAKE_Core_MODULE_MAJOR_VERSION}.${CMAKE_Core_MODULE_MINOR_VERSION}
    REQUIRED COMPONENTS Qml
)

set(CMAKE_AUTOMOC ON)

# A target created with plain add_executable() instead of qt_add_executable()
# never runs Qt's executable finalizers, and so never gets a build-time QML
# import scan. What the QML module sets up for it - the qt.conf recording its
# import paths, in particular - must then not depend on that scan's output.
add_executable(test_unfinalized_executable main.cpp)
target_link_libraries(test_unfinalized_executable PRIVATE Qt6::Qml)

qt_add_qml_module(test_unfinalized_executable
    URI org.qt-project.test_unfinalized_executable
    VERSION 1.0
    QML_FILES
        Main.qml
)
