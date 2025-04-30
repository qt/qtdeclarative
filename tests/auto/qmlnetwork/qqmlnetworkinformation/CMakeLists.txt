# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if (NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qqmlnetworkinformation LANGUAGES C CXX)
    find_package(Qt6BuildInternals COMPONENTS STANDALONE_TEST)
endif()

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/data/tst_*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_qqmlnetworkinformation
    QMLTEST
    SOURCES
        tst_qqmlnetworkinformation.cpp
    LIBRARIES
        Qt::Qml
        Qt::QmlPrivate
        Qt::QuickTestUtilsPrivate
        Qt::Network
    TESTDATA ${test_data}
)

if(QT_BUILD_STANDALONE_TESTS)
    qt_import_qml_plugins(tst_qqmlnetworkinformation)
endif()

qt_internal_extend_target(tst_qqmlnetworkinformation CONDITION ANDROID OR IOS
    DEFINES
        QT_QMLTEST_DATADIR=":/data"
)

qt_internal_extend_target(tst_qqmlnetworkinformation CONDITION NOT ANDROID AND NOT IOS
    DEFINES
        QT_QMLTEST_DATADIR="${CMAKE_CURRENT_SOURCE_DIR}/data"
)
