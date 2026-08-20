# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if (NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(dialogs LANGUAGES C CXX)
    find_package(Qt6BuildInternals COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_manual_test(tst_manual_dialogs
    GUI
    SOURCES
        dialogs.cpp
    LIBRARIES
        Qt::Gui
        Qt::Qml
        Qt::QuickControls2
)

# Resources:
set(qmake_immediate_resource_files
    "dialogs.qml"
    "ColorDialogPage.qml"
    "FileDialogPage.qml"
    "FolderDialogPage.qml"
    "FontDialogPage.qml"
    "MessageDialogPage.qml"
    "StringListView.qml"
    "qmldir"
    "Theme.qml"
)

qt_internal_add_resource(tst_manual_dialogs "qmake_immediate"
    PREFIX
        "/"
    FILES
        ${qmake_immediate_resource_files}
)
