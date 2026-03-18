# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_manual_overridesemantics LANGUAGES CXX)
    find_package(Qt6 REQUIRED COMPONENTS Core Qml)
    qt_standard_project_setup(REQUIRES 6.11)
endif()

qt_internal_add_manual_test(tst_manual_overridesemantics
    SOURCES
        main.cpp
    LIBRARIES
        Qt::Core
        Qt::Qml
)

qt_add_qml_module(tst_manual_overridesemantics
    URI OverrideSemantics
    QML_FILES
        QmlPlainBase.qml
        QmlVirtualBase.qml
        QmlOverrideBase.qml
        QmlFinalBase.qml
        QmlMethodBase.qml
    SOURCES
        cpptypes.h
)
