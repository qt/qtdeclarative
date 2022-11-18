# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if (NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(qquickdialog LANGUAGES C CXX ASM)
    find_package(Qt6BuildInternals COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_manual_test(qquickdialog
    GUI
    SOURCES
        qquickdialog.cpp
    LIBRARIES
        Qt::Gui
        Qt::Qml
        Qt::QuickControls2
)

# Resources:
set(qmake_immediate_resource_files
    "CustomDialog.qml"
    "DialogLabel.qml"
    "Marker.qml"
    "qquickdialog.qml"
    "qtquickcontrols2.conf"
)

qt_internal_add_resource(qquickdialog "qmake_immediate"
    PREFIX
        "/"
    FILES
        ${qmake_immediate_resource_files}
)
