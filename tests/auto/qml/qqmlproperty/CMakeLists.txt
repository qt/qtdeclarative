# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from qqmlproperty.pro.

#####################################################################
## tst_qqmlproperty Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    data/*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_qqmlproperty
    SOURCES
        interfaces.h
        tst_qqmlproperty.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::QmlPrivate
        Qt::QuickTestUtilsPrivate
    TESTDATA ${test_data}
)

#### Keys ignored in scope 1:.:.:qqmlproperty.pro:<TRUE>:
# QML_IMPORT_NAME = "io.qt.bugreports"
# QML_IMPORT_VERSION = "2.0"

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qqmlproperty CONDITION ANDROID OR IOS
    DEFINES
        QT_QMLTEST_DATADIR=":/data"
)

qt_internal_extend_target(tst_qqmlproperty CONDITION NOT ANDROID AND NOT IOS
    DEFINES
        QT_QMLTEST_DATADIR="${CMAKE_CURRENT_SOURCE_DIR}/data"
)

set_target_properties(tst_qqmlproperty PROPERTIES
    QT_QML_MODULE_VERSION 2.0
    QT_QML_MODULE_URI io.qt.bugreports
)

_qt_internal_qml_type_registration(tst_qqmlproperty)
