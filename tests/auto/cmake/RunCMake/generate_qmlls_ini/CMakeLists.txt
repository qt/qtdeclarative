# Copyright (C) 2025 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(${RunCMake_TEST} LANGUAGES CXX)

find_package(Qt6 REQUIRED COMPONENTS Qml Core)

# Setting AUTOMOC to true to make sure Qt package is used
set(CMAKE_AUTOMOC ON)

qt_standard_project_setup(REQUIRES 6.8)
qt_add_library(generate_qmlls_ini STATIC)
qt_add_qml_module(generate_qmlls_ini
    URI generate_qmlls_ini
    QML_FILES Main.qml
)
