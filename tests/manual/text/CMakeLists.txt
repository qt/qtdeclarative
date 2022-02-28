# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

cmake_minimum_required(VERSION 3.16)
project(text_manual_test LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_AUTOUIC ON)

find_package(Qt6 COMPONENTS Core Gui Quick QuickControls2)

qt_add_executable(text_manual_test
    main.cpp
)
target_link_libraries(text_manual_test PUBLIC
        Qt::Gui
        Qt::Quick
        Qt::QuickControls2
)

set(qmake_immediate_resource_files
    "Rule.qml"
    "SignalIndicator.qml"
    "textInputPropertiesAndSignals.qml"
)

qt6_add_resources(text_manual_test "qml"
    PREFIX
        "/"
    FILES
        ${qmake_immediate_resource_files}
)
