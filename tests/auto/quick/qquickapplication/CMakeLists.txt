# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from qquickapplication.pro.

#####################################################################
## tst_qquickapplication Test:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qquickapplication LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    data/*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_qquickapplication
    SOURCES
        tst_qquickapplication.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::Qml
        Qt::QmlPrivate
        Qt::Quick
        Qt::QuickPrivate
        Qt::QuickTestUtilsPrivate
    TESTDATA ${test_data}
)

#### Keys ignored in scope 1:.:.:qquickapplication.pro:<TRUE>:
# OTHER_FILES = "data/tst_displayname.qml" "data/tst_platformname.qml"

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qquickapplication CONDITION ANDROID OR IOS
    DEFINES
        QT_QMLTEST_DATADIR=":/data"
)

qt_internal_extend_target(tst_qquickapplication CONDITION NOT ANDROID AND NOT IOS
    DEFINES
        QT_QMLTEST_DATADIR="${CMAKE_CURRENT_SOURCE_DIR}/data"
)
