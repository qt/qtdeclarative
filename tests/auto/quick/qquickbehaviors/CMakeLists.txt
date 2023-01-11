# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from qquickbehaviors.pro.

#####################################################################
## tst_qquickbehaviors Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    data/*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_qquickbehaviors
    SOURCES
        tst_qquickbehaviors.cpp
        bindable.h
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::QmlPrivate
        Qt::QuickPrivate
        Qt::QuickTestUtilsPrivate
    TESTDATA ${test_data}
)

set_target_properties(tst_qquickbehaviors PROPERTIES
    QT_QML_MODULE_VERSION 1.0
    QT_QML_MODULE_URI test
)

_qt_internal_qml_type_registration(tst_qquickbehaviors)


#### Keys ignored in scope 1:.:.:qquickbehaviors.pro:<TRUE>:
# DISTFILES = "data/*"

## Scopes:
#####################################################################

qt_internal_extend_target(tst_qquickbehaviors CONDITION ANDROID OR IOS
    DEFINES
        QT_QMLTEST_DATADIR=":/data"
)

qt_internal_extend_target(tst_qquickbehaviors CONDITION NOT ANDROID AND NOT IOS
    DEFINES
        QT_QMLTEST_DATADIR="${CMAKE_CURRENT_SOURCE_DIR}/data"
)
