# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from qquickdesignersupport.pro.

#####################################################################
## tst_qquickdesignersupport Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    data/*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_qquickdesignersupport
    SOURCES
        tst_qquickdesignersupport.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::QmlPrivate
        Qt::QuickPrivate
        Qt::QuickTestUtilsPrivate
    TESTDATA ${test_data}
)

#### Keys ignored in scope 1:.:.:qquickdesignersupport.pro:<TRUE>:
# DISTFILES = "data/TestComponent.qml" "data/test.qml" "data/componentTest.qml" "data/Component01.qml" "data/Component02.qml" "data/bindingTest.qml"

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qquickdesignersupport CONDITION ANDROID OR IOS
    DEFINES
        QT_QMLTEST_DATADIR=":/data"
)

qt_internal_extend_target(tst_qquickdesignersupport CONDITION NOT ANDROID AND NOT IOS
    DEFINES
        QT_QMLTEST_DATADIR="${CMAKE_CURRENT_SOURCE_DIR}/data"
)
