# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from qqmlimport.pro.

#####################################################################
## tst_qqmlimport Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    data/*)
list(APPEND test_data ${test_data_glob})
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    MyPluginSupported/*)
list(APPEND test_data ${test_data_glob})
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    MyPluginUnsupported/*)
list(APPEND test_data ${test_data_glob})
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    FormFromQmlDir/*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_qqmlimport
    SOURCES
        tst_qqmlimport.cpp
    LIBRARIES
        Qt::Gui
        Qt::Qml
        Qt::QmlPrivate
        Qt::Quick
        Qt::QuickTestUtilsPrivate
    TESTDATA ${test_data}
)

qt_internal_add_resource(tst_qqmlimport "preferred"
    PREFIX
        "/qqmlimport/ModuleWithPrefer/"
    FILES
        "Preferred.qml"
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qqmlimport CONDITION ANDROID OR IOS
    DEFINES
        QT_QMLTEST_DATADIR=":/data"
)

qt_internal_extend_target(tst_qqmlimport CONDITION NOT ANDROID AND NOT IOS
    DEFINES
        QT_QMLTEST_DATADIR="${CMAKE_CURRENT_SOURCE_DIR}/data"
)
