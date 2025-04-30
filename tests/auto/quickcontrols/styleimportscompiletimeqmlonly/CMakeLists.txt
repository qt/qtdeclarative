# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from styleimportscompiletimeqmlonly.pro.

if (NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_styleimportscompiletimeqmlonly LANGUAGES C CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

#####################################################################
## tst_styleimportscompiletimeqmlonly Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    data/*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_styleimportscompiletimeqmlonly
    SOURCES
        tst_styleimportscompiletimeqmlonly.cpp
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
        Qt::QuickControlsTestUtilsPrivate
        Qt::QuickPrivate
        Qt::QuickTemplates2Private
        Qt::QuickTest
        Qt::QuickTestUtilsPrivate
        Qt::TestPrivate
    TESTDATA ${test_data}
)

#### Keys ignored in scope 1:.:.:styleimportscompiletimeqmlonly.pro:<TRUE>:
# OTHER_FILES = "data/*.qml" "data/QmlOnly/*.qml" "data/QmlOnly/qmldir"

## Scopes:
#####################################################################

qt_internal_extend_target(tst_styleimportscompiletimeqmlonly CONDITION ANDROID OR IOS
    DEFINES
        QT_QMLTEST_DATADIR=":/data"
)

qt_internal_extend_target(tst_styleimportscompiletimeqmlonly CONDITION NOT ANDROID AND NOT IOS
    DEFINES
        QT_QMLTEST_DATADIR="${CMAKE_CURRENT_SOURCE_DIR}/data"
)
