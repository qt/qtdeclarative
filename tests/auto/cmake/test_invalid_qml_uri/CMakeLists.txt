# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

cmake_minimum_required(VERSION 3.16)

project(test_invalid_qml_uri LANGUAGES CXX)

find_package(Qt6 COMPONENTS Qml)

qt_standard_project_setup()

qt_add_qml_module(test_invalid_qml_uri
    URI 01test
)
