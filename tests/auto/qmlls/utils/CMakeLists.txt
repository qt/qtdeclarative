# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qmlls_utils LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    data)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_qmlls_utils
    SOURCES
    tst_qmlls_utils.cpp
    DEFINES
        QT_QMLLS_UTILS_DATADIR="${CMAKE_CURRENT_SOURCE_DIR}/data"
    LIBRARIES
        Qt::Core
        Qt::QmlDomPrivate
        Qt::LanguageServerPrivate
        Qt::Test
        Qt::QuickTestUtilsPrivate
        Qt::QmlLSPrivate
    TESTDATA ${test_data}
)

qt_internal_add_test(tst_qmlls_highlighting
    SOURCES
    tst_qmlls_highlighting.cpp
    DEFINES
        QT_QMLLS_HIGHLIGHTS_DATADIR="${CMAKE_CURRENT_SOURCE_DIR}/data"
    LIBRARIES
        Qt::Core
        Qt::QmlDomPrivate
        Qt::LanguageServerPrivate
        Qt::Test
        Qt::QuickTestUtilsPrivate
        Qt::QmlLSPrivate
    TESTDATA ${test_data}
)

qt_internal_add_test(tst_qmlls_documentationHints
    SOURCES
    tst_qmlls_documentationHints.cpp
    DEFINES
        QT_QMLLS_DOCUMENTATION_DATADIR="${CMAKE_CURRENT_SOURCE_DIR}/data"
    LIBRARIES
        Qt::Core
        Qt::QmlDomPrivate
        Qt::LanguageServerPrivate
        Qt::Test
        Qt::QuickTestUtilsPrivate
        Qt::QmlLSPrivate
    TESTDATA ${test_data}
)

qt_internal_extend_target(tst_qmlls_utils CONDITION ANDROID OR IOS
    DEFINES
        QT_QMLLS_UTILS_DATADIR=":/domdata"
)
