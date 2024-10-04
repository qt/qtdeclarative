# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause


cmake_minimum_required(VERSION 3.19)

project(test_internal_singleton)

find_package(Qt6 REQUIRED COMPONENTS Qml)

qt_standard_project_setup()

set_source_files_properties(Test.qml PROPERTIES
    QT_QML_SINGLETON_TYPE TRUE
    QT_QML_INTERNAL_TYPE TRUE
)

qt_add_qml_module(test_internal_singleton
    URI Controls
    VERSION 1.0
    QML_FILES
    Test.qml
)
