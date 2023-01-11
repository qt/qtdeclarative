# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from qquickgraphicsinfo.pro.

#####################################################################
## tst_qquickgraphicsinfo Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    data/*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_qquickgraphicsinfo
    SOURCES
        tst_qquickgraphicsinfo.cpp
    LIBRARIES
        Qt::Gui
        Qt::Quick
        Qt::QuickTestUtilsPrivate
    TESTDATA ${test_data}
)

#### Keys ignored in scope 1:.:.:qquickgraphicsinfo.pro:<TRUE>:
# OTHER_FILES = "data/basic.qml"

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qquickgraphicsinfo CONDITION ANDROID OR IOS
    DEFINES
        QT_QMLTEST_DATADIR=":/data"
)

qt_internal_extend_target(tst_qquickgraphicsinfo CONDITION NOT ANDROID AND NOT IOS
    DEFINES
        QT_QMLTEST_DATADIR="${CMAKE_CURRENT_SOURCE_DIR}/data"
)
