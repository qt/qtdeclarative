# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from accessibility.pro.

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_accessibility LANGUAGES C CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

if(NOT QT_FEATURE_accessibility)
    return()
endif()

#####################################################################
## tst_accessibility Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    data/*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_accessibility
    SOURCES
        tst_accessibility.cpp
    DEFINES
        QQC2_IMPORT_PATH="${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/quickcontrols"
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::Qml
        Qt::QmlPrivate
        Qt::QuickControls2
        Qt::QuickControls2Private
        Qt::QuickPrivate
        Qt::QuickTemplates2Private
        Qt::QuickTest
        Qt::QuickTestUtilsPrivate
        Qt::TestPrivate
    TESTDATA ${test_data}
)

#### Keys ignored in scope 1:.:.:accessibility.pro:<TRUE>:
# OTHER_FILES = "data/*.qml"
# _REQUIREMENTS = "qtConfig(accessibility)"

## Scopes:
#####################################################################

qt_internal_extend_target(tst_accessibility CONDITION ANDROID OR IOS
    DEFINES
        QT_QMLTEST_DATADIR=":/data"
)

qt_internal_extend_target(tst_accessibility CONDITION NOT ANDROID AND NOT IOS
    DEFINES
        QT_QMLTEST_DATADIR="${CMAKE_CURRENT_SOURCE_DIR}/data"
)
