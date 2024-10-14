# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_how-to-cpp-gauge LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

set(CMAKE_AUTOUIC OFF)

qt_standard_project_setup(REQUIRES 6.8)

qt_internal_add_test(tst_how-to-cpp-gauge
    SOURCES
        tst_how-to-cpp-gauge.cpp
    LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::Quick
        Qt::QuickPrivate
        Qt::QuickControlsTestUtilsPrivate
        Qt::QuickTemplates2Private
)

qt_add_qml_module(tst_how-to-cpp-gauge
    URI GaugeHowTo
    DEPENDENCIES
        QtQuick
    QML_FILES
        Gauge.qml
        Main.qml
    SOURCES
        minortickmarkcontainer.cpp
        minortickmarkcontainer.h
        tickmarkcontainer.cpp
        tickmarkcontainer.h
        tickmarklabelcontainer.cpp
        tickmarklabelcontainer.h
        valuerange.cpp
        valuerange.h
)
