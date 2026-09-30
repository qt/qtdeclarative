# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_deltablue LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_benchmark(tst_deltablue
    SOURCES
        main.cpp
    LIBRARIES
        Qt::Qml
        Qt::Test
)


set_source_files_properties(
        Main.qml
        Strength.qml
    PROPERTIES
        QT_QML_SINGLETON_TYPE True
)

qt_add_qml_module(tst_deltablue
    URI DeltaBlue
    RESOURCE_PREFIX "/"
    QML_FILES
        BaseConstraint.qml
        ChainTest.qml
        Constraint.qml
        Main.qml
        Main2.qml
        Plan.qml
        Planner.qml
        ProjectionTest.qml
        Satisfaction.qml
        Strength.qml
        Variable.qml
        deltablue.js
)

