# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from qqmlinfo.pro.

#####################################################################
## tst_qqmlinfo Test:
#####################################################################

if(NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(tst_qqmlinfo LANGUAGES CXX)
    find_package(Qt6BuildInternals REQUIRED COMPONENTS STANDALONE_TEST)
endif()

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    data/*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_qqmlinfo
    SOURCES
        attached.cpp attached.h
        tst_qqmlinfo.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::Qml
        Qt::QmlPrivate
        Qt::QuickTestUtilsPrivate
    TESTDATA ${test_data}
)

if(QT_BUILD_STANDALONE_TESTS)
    qt_import_qml_plugins(tst_qqmlinfo)
endif()

#### Keys ignored in scope 1:.:.:qqmlinfo.pro:<TRUE>:
# QML_IMPORT_MAJOR_VERSION = "1"
# QML_IMPORT_NAME = "org.qtproject.Test"

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qqmlinfo CONDITION ANDROID OR IOS
    DEFINES
        QT_QMLTEST_DATADIR=":/data"
)

qt_internal_extend_target(tst_qqmlinfo CONDITION NOT ANDROID AND NOT IOS
    DEFINES
        QT_QMLTEST_DATADIR="${CMAKE_CURRENT_SOURCE_DIR}/data"
)

set_target_properties(tst_qqmlinfo PROPERTIES
    QT_QML_MODULE_VERSION 1.0
    QT_QML_MODULE_URI org.qtproject.Test
)

_qt_internal_qml_type_registration(tst_qqmlinfo)
