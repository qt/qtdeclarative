# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

cmake_minimum_required(VERSION 3.16)
project(tableofcontents LANGUAGES CXX)

set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.8 REQUIRED COMPONENTS Quick)

qt_standard_project_setup(REQUIRES 6.8)

qt_add_executable(apptableofcontents
    main.cpp
)

qt_add_resources(apptableofcontents "contentdata"
    FILES
        content.txt
        arrow_icon.png
)

qt_add_qml_module(apptableofcontents
    URI tableofcontents
    VERSION 1.0
    QML_FILES
        Main.qml
    SOURCES
        treeitem.h
        treeitem.cpp
        treemodel.h
        treemodel.cpp
)

target_link_libraries(apptableofcontents
    PRIVATE Qt6::Quick
)
