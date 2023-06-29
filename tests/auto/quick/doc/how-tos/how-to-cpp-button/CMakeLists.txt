# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_how-to-cpp-button LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_test(tst_how-to-cpp-button
    SOURCES
        tst_how-to-cpp-button.cpp
    LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::Quick
        Qt::QuickPrivate
        Qt::QuickControlsTestUtilsPrivate
        Qt::QuickTemplates2Private
)

qt_policy(SET QTP0001 NEW)

qt_add_qml_module(tst_how-to-cpp-button
    URI MyModule
    QML_FILES
        Main.qml
    SOURCES
        backend.h
        backend.cpp
)
