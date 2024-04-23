# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
cmake_minimum_required(VERSION 3.16)

project(qml_in_java_based_android_project VERSION 0.1 LANGUAGES CXX)

install(DIRECTORY
    gradle
    app
    DESTINATION .
)
install(FILES
    settings.gradle
    gradle.properties
    build.gradle
    CMakeLists.txt
    DESTINATION .
)
