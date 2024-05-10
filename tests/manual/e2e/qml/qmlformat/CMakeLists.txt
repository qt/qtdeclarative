# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## e2e_qmlformat Test:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qmlformat LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

qt_internal_add_test(e2e_qmlformat
    SOURCES
        e2e_qmlformat.cpp
    DEFINES
        SRCDIR="${CMAKE_CURRENT_SOURCE_DIR}"
    LIBRARIES
        Qt::Core
        Qt::QmlDomPrivate
        Qt::TestPrivate
        Qt::QuickTestUtilsPrivate
    TIMEOUT 3000
)

add_dependencies(e2e_qmlformat Qt::qmlformat)
