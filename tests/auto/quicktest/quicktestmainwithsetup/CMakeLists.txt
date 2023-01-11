# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from quicktestmainwithsetup.pro.

#####################################################################
## tst_quicktestmainwithsetup Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/data/*.qml)
list(APPEND test_data ${test_data_glob})

file(GLOB_RECURSE test_imports_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    ${CMAKE_CURRENT_SOURCE_DIR}/imports/*)
list(APPEND test_data ${test_imports_glob})

qt_internal_add_test(tst_quicktestmainwithsetup
    QMLTEST
    SOURCES
        tst_quicktestmainwithsetup.cpp
    LIBRARIES
        Qt::Gui
        Qt::Quick
        Qt::QuickTestUtilsPrivate
    TESTDATA ${test_data}
)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_quicktestmainwithsetup CONDITION ANDROID OR IOS
    DEFINES
        QT_QMLTEST_DATADIR=":/data"
)

qt_internal_extend_target(tst_quicktestmainwithsetup CONDITION WEBOS
    DEFINES
        QT_QMLTEST_DATADIR="./data"
)

qt_internal_extend_target(tst_quicktestmainwithsetup CONDITION NOT ANDROID AND NOT IOS AND NOT WEBOS
    DEFINES
        QT_QMLTEST_DATADIR="${CMAKE_CURRENT_SOURCE_DIR}/data"
)
