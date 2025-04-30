# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if (NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(workerscript LANGUAGES C CXX)
    find_package(Qt6BuildInternals COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_manual_test(tst_manual_workerscript
    GUI
    SOURCES
        workerscript.cpp
    DEFINES
        QT_DEPRECATED_WARNINGS
    LIBRARIES
        Qt::Gui
        Qt::Qml
        Qt::Quick
)

# Resources:
set(qmake_immediate_resource_files
    "Spinner.qml"
    "workerscript.qml"
    "workerscript.mjs"
)

qt_internal_add_resource(tst_manual_workerscript "qmake_immediate"
    PREFIX
        "/qt/qml/workerscript"
    FILES
        ${qmake_immediate_resource_files}
)
