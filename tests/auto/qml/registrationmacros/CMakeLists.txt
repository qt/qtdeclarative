# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_registrationmacros LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_test(tst_registrationmacros
    SOURCES
        tst_registrationmacros.cpp
        types.h
        types.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::Qml
        Qt::QmlPrivate
)

set_target_properties(tst_registrationmacros PROPERTIES
    QT_QML_MODULE_VERSION 6.2
    QT_QML_MODULE_URI test
)

_qt_internal_qml_type_registration(tst_registrationmacros)
