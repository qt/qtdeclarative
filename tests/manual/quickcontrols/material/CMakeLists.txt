# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if (NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(material LANGUAGES C CXX)
    find_package(Qt6BuildInternals COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_manual_test(material
    GUI
    SOURCES
        material.cpp
    LIBRARIES
        Qt::Gui
        Qt::Qml
        Qt::QuickControls2
)

set(qmake_immediate_resource_files
    "material.qml"
    "Constants.qml"
    "pages/ButtonPage.qml"
    "pages/DelayButtonPage.qml"
    "pages/RoundButtonPage.qml"
    "pages/SwitchPage.qml"
    "pages/TextAreaPage.qml"
    "pages/TextFieldPage.qml"
    "qmldir"
)

qt_internal_add_resource(material "qmake_immediate"
    PREFIX
        "/"
    FILES
        ${qmake_immediate_resource_files}
)
