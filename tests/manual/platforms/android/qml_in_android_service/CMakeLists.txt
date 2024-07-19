# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause
cmake_minimum_required(VERSION 3.16)

project(qml_in_android_service VERSION 0.1 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.7 REQUIRED COMPONENTS Quick)

qt_standard_project_setup(REQUIRES 6.6)

qt_add_executable(qml_in_android_service
    main.cpp
)

qt_add_qml_module(qml_in_android_service
    URI qml_floating_view
    VERSION 1.0
    QML_FILES Main.qml
)

target_link_libraries(qml_in_android_service
    PRIVATE Qt6::Quick
)

install(TARGETS qml_in_android_service
    BUNDLE DESTINATION .
    RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

