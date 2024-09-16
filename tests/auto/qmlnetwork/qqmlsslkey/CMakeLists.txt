# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_sslkey_qml LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_test(tst_sslkey_qml
    QMLTEST
    SOURCES
        tst_sslkey_qml.cpp
    LIBRARIES
        Qt::Qml
        Qt::QuickTestUtilsPrivate
        Qt::QmlNetwork
)

qt_policy(SET QTP0001 NEW)

qt_add_qml_module(tst_sslkey_qml
    URI QmlTestUri
    VERSION 1.0
    NO_GENERATE_EXTRA_QMLDIRS
    QML_FILES
        qml/tst_sslkey.qml
)

qt_autogen_tools_initial_setup(tst_sslkey_qml)
