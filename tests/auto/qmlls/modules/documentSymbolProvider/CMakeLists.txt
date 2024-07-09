# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_document_symbol_utils LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_test(tst_document_symbol_utils
    SOURCES
        tst_document_symbol_utils.h
        tst_document_symbol_utils.cpp
    LIBRARIES
        Qt::QmlDomPrivate
        Qt::QmlLSPrivate
        Qt::LanguageServerPrivate
        Qt::Test
)
