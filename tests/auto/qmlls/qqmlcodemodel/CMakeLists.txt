# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qmlls_qqmlcodemodel LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    data)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_qmlls_qqmlcodemodel
    SOURCES
        tst_qmlls_qqmlcodemodel.cpp tst_qmlls_qqmlcodemodel.h
    LIBRARIES
        Qt::Core
        Qt::QmlDomPrivate
        Qt::LanguageServerPrivate
        Qt::Test
        Qt::QuickTestUtilsPrivate
        Qt::QmlLSPrivate
    TESTDATA ${test_data}
    DEFINES
        QT_QQMLCODEMODEL_DATADIR="${CMAKE_CURRENT_SOURCE_DIR}/data"
)

qt_internal_extend_target(tst_qmlls_qqmlcodemodel CONDITION ANDROID OR IOS
    DEFINES
        QT_QQMLCODEMODEL_DATADIR=":/data"
)
