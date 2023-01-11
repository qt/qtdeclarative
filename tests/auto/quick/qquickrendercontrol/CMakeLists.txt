# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from qquickrendercontrol.pro.

#####################################################################
## tst_qquickrendercontrol Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
        RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
        data/*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_qquickrendercontrol
    SOURCES
        tst_qquickrendercontrol.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::QmlPrivate
        Qt::QuickPrivate
        Qt::QuickTestUtilsPrivate
    TESTDATA ${test_data}
)

#### Keys ignored in scope 1:.:.:qquickrendercontrol.pro:<TRUE>:
# OTHER_FILES = "data/rect.qml"

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qquickrendercontrol CONDITION ANDROID OR IOS
    DEFINES
        QT_QMLTEST_DATADIR=":/data"
)

qt_internal_extend_target(tst_qquickrendercontrol CONDITION NOT ANDROID AND NOT IOS
    DEFINES
        QT_QMLTEST_DATADIR="${CMAKE_CURRENT_SOURCE_DIR}/data"
)
