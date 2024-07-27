# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
cmake_minimum_required(VERSION 3.16)

project(qtquickview VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.8 REQUIRED COMPONENTS Quick)

qt_standard_project_setup(REQUIRES 6.8)

qt_add_executable(qtquickview
    main.cpp
)

qt_add_qml_module(qtquickview
    URI qmlModule
    VERSION 1.0
    QML_FILES Main.qml
    QML_FILES Second.qml
)

target_link_libraries(qtquickview
    PRIVATE Qt6::Quick
)

install(TARGETS qtquickview
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

