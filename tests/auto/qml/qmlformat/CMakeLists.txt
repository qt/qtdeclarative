# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from qmlformat.pro.

#####################################################################
## tst_qmlformat Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    data/*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_qmlformat
    SOURCES
        tst_qmlformat.cpp
    DEFINES
        SRCDIR="${CMAKE_CURRENT_SOURCE_DIR}"
    LIBRARIES
        Qt::Core
        Qt::QmlDomPrivate
        Qt::TestPrivate
        Qt::QuickTestUtilsPrivate
    TESTDATA ${test_data}
    TIMEOUT 3000
)

add_dependencies(tst_qmlformat Qt::qmlformat)

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qmlformat CONDITION ANDROID OR IOS
    DEFINES
        QT_QMLTEST_DATADIR=":/data"
)

qt_internal_extend_target(tst_qmlformat CONDITION NOT ANDROID AND NOT IOS
    DEFINES
        QT_QMLTEST_DATADIR="${CMAKE_CURRENT_SOURCE_DIR}/data"
)
