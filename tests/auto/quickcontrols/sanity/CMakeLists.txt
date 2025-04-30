# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from sanity.pro.

if (NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_sanity LANGUAGES C CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    data/*)
list(APPEND test_data ${test_data_glob})

#####################################################################
## tst_sanity Test:
#####################################################################

add_library(QuickControlsSanityPlugin STATIC quickcontrolssanity.h quickcontrolssanity.cpp)
target_link_libraries(QuickControlsSanityPlugin PRIVATE Qt::QmlCompilerPrivate)
qt_autogen_tools_initial_setup(QuickControlsSanityPlugin)
target_compile_definitions(QuickControlsSanityPlugin PRIVATE QT_STATICPLUGIN)

qt_internal_add_test(tst_sanity
    SOURCES
        tst_sanity.cpp
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
        Qt::QmlCompilerPrivate
        QuickControlsSanityPlugin
    TESTDATA ${test_data}
)

#### Keys ignored in scope 1:.:.:sanity.pro:<TRUE>:
# OTHER_FILES = "$$PWD/BLACKLIST"
# TEMPLATE = "app"

## Scopes:
#####################################################################

qt_internal_extend_target(tst_sanity CONDITION ANDROID OR IOS
    DEFINES
        QT_QMLTEST_DATADIR=":/data"
)

qt_internal_extend_target(tst_sanity CONDITION NOT ANDROID AND NOT IOS
    DEFINES
        QT_QMLTEST_DATADIR="${CMAKE_CURRENT_SOURCE_DIR}/data"
)

qt_internal_extend_target(tst_sanity CONDITION WEBOS
    DEFINES
        QQC2_IMPORT_PATH="/usr/lib/qml/QtQuick/Controls"
)
