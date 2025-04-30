# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from qquickiconimage.pro.

if (NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qquickiconimage LANGUAGES C CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

#####################################################################
## tst_qquickiconimage Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/data/*.qml)
list(APPEND test_data ${test_data_glob})
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/data/icons/*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_qquickiconimage
    SOURCES
        tst_qquickiconimage.cpp
    DEFINES
        QQC2_IMPORT_PATH="${CMAKE_CURRENT_SOURCE_DIR}/../../../../src/quickcontrols"
    LIBRARIES
        Qt::QuickControls2ImplPrivate
        Qt::QuickPrivate
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::Qml
        Qt::QmlPrivate
        Qt::Quick
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

# Resources:
set(resources_resource_files
    "icons/testtheme/16x16/actions/appointment-new.png"
    "icons/testtheme/22x22/actions/+testselector/appointment-new.png"
    "icons/testtheme/22x22/actions/+testselector/appointment-new@2x.png"
    "icons/testtheme/22x22/actions/appointment-new.png"
    "icons/testtheme/22x22/actions/appointment-new@2x.png"
    "icons/testtheme/22x22/actions/color-test-original.png"
    "icons/testtheme/22x22/actions/color-test-original@2x.png"
    "icons/testtheme/22x22/actions/color-test-tinted.png"
    "icons/testtheme/22x22/actions/color-test-tinted@2x.png"
    "icons/testtheme/22x22@2/actions/+testselector/appointment-new.png"
    "icons/testtheme/22x22@2/actions/appointment-new.png"
    "icons/testtheme/appointment-new.svg"
    "icons/testtheme/index.theme"
)

qt_internal_add_resource(tst_qquickiconimage "resources"
    PREFIX
        "/"
    FILES
        ${resources_resource_files}
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qquickiconimage CONDITION TARGET Qt::Svg
    LIBRARIES
        Qt::Svg
)

qt_internal_extend_target(tst_qquickiconimage CONDITION ANDROID OR IOS
    DEFINES
        QT_QMLTEST_DATADIR=":/data"
)

qt_internal_extend_target(tst_qquickiconimage CONDITION NOT ANDROID AND NOT IOS
    DEFINES
        QT_QMLTEST_DATADIR="${CMAKE_CURRENT_SOURCE_DIR}/data"
)
