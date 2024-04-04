# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)

project(nontoplevelrequiredproperty)

find_package(Qt6 REQUIRED Quick REQUIRED ${Qt6Tests_PREFIX_PATH})

qt_standard_project_setup()

qt_add_executable(executable
     main.cpp
)

qt6_add_qml_module(executable
    VERSION 1.0
    URI QmltcBuildFailures
    QML_FILES
        Main.qml
    DEPENDENCIES
        QtQuick
    ENABLE_TYPE_COMPILER
)

target_link_libraries(executable PRIVATE Qt6::Quick)
