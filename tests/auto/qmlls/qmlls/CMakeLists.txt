# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qmlls LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    data)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_qmlls
    SOURCES
        tst_qmlls.cpp
    DEFINES
        QT_DEPRECATED_WARNINGS
        QT_QMLTEST_DATADIR="${CMAKE_CURRENT_SOURCE_DIR}/data"
    LIBRARIES
        Qt::Core
        Qt::QmlDomPrivate
        Qt::LanguageServerPrivate
        Qt::Test
        Qt::QuickTestUtilsPrivate
    TESTDATA ${test_data}
)

qt_internal_extend_target(tst_qmlls CONDITION ANDROID OR IOS
    DEFINES
        QT_QMLTEST_DATADIR=":/domdata"
)

# The test spawns qmlls as a subprocess; it needs the qmlls completion plugins
# and the QtQuick qmltypes from the build tree.
foreach(_qmlls_test_dep IN ITEMS qmlls qt_qmlls_plugins_all Quick)
    if (TARGET ${_qmlls_test_dep})
        add_dependencies(tst_qmlls ${_qmlls_test_dep})
    endif()
endforeach()
