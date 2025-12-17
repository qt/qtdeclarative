
# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if (NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(textManual LANGUAGES C CXX)
    find_package(Qt6BuildInternals COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_manual_test(qml_basic_item
    GUI
    SOURCES
        main.cpp
    LIBRARIES
        Qt::Gui
        Qt::Qml
        Qt::QuickControls2
)

# Resources:
set(qmake_immediate_resource_files
    "main.qml"
    "WasmMenu.qml"
    "WasmToolBar.qml"
    "MeetingTabs.qml"
    "MeetingInviteesPage.qml"
    "MeetingSchedulerPage.qml"
    "MeetingSummary.qml"
    "WasmListView.qml"
    "MeetingInviteesModel.qml"
    "AboutDialog.qml"
)

add_custom_command(
    TARGET qml_basic_item
    PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E copy_if_different
            ${CMAKE_CURRENT_SOURCE_DIR}/basic_items.html
            ${CMAKE_CURRENT_BINARY_DIR}/basic_items.html
)

set(CMAKE_CXX_FLAGS_DEBUG "-g0 -O3")
qt6_import_qml_plugins(qml_basic_item INCLUDE Qt::QtQuick2Plugin)
qt_internal_add_resource (qml_basic_item "qmake_immediate"
    PREFIX
        "/"
    FILES
        ${qmake_immediate_resource_files}
)

