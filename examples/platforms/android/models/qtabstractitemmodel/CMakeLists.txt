# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause
cmake_minimum_required(VERSION 3.16)

project(qtabstractitemmodel VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.8 REQUIRED COMPONENTS Quick)

qt_standard_project_setup(REQUIRES 6.8)

qt_add_executable(qtabstractitemmodel
    main.cpp
)

qt_add_qml_module(qtabstractitemmodel
    URI QmlModule
    VERSION 1.0
    QML_FILES Main.qml
)

target_link_libraries(qtabstractitemmodel PRIVATE
    Qt6::Quick
)

include(GNUInstallDirs)
install(TARGETS qtabstractitemmodel
    BUNDLE DESTINATION .
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)
