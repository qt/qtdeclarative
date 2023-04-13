# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from qqmlbinding.pro.

#####################################################################
## tst_qqmlbinding Test:
#####################################################################

# Collect test data
file(GLOB_RECURSE test_data_glob
    RELATIVE ${CMAKE_CURRENT_SOURCE_DIR}
    data/*)
list(APPEND test_data ${test_data_glob})

qt_internal_add_test(tst_qqmlbinding
    SOURCES
        tst_qqmlbinding.cpp
        WithBindableProperties.h
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::QmlPrivate
        Qt::QuickPrivate
        Qt::QuickTestUtilsPrivate
    TESTDATA ${test_data}
)

qt_internal_add_test(tst_qqmlbinding_no_deferred_properties
    SOURCES
        tst_qqmlbinding.cpp
        WithBindableProperties.h
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::QmlPrivate
        Qt::QuickPrivate
        Qt::QuickTestUtilsPrivate
    TESTDATA ${test_data}
    DEFINES
        QML_DISABLE_INTERNAL_DEFERRED_PROPERTIES
)

set_target_properties(tst_qqmlbinding PROPERTIES
    QT_QML_MODULE_URI "test"
    QT_QML_MODULE_VERSION 1.0
)

_qt_internal_qml_type_registration(tst_qqmlbinding)

set_target_properties(tst_qqmlbinding_no_deferred_properties PROPERTIES
    QT_QML_MODULE_URI "test"
    QT_QML_MODULE_VERSION 1.0
)

_qt_internal_qml_type_registration(tst_qqmlbinding_no_deferred_properties)


qt_internal_extend_target(tst_qqmlbinding CONDITION ANDROID OR IOS
    DEFINES
        QT_QMLTEST_DATADIR=":/data"
)

qt_internal_extend_target(tst_qqmlbinding CONDITION NOT ANDROID AND NOT IOS
    DEFINES
        QT_QMLTEST_DATADIR="${CMAKE_CURRENT_SOURCE_DIR}/data"
)

qt_internal_extend_target(tst_qqmlbinding_no_deferred_properties CONDITION ANDROID OR IOS
    DEFINES
        QT_QMLTEST_DATADIR=":/data"
)

qt_internal_extend_target(tst_qqmlbinding_no_deferred_properties CONDITION NOT ANDROID AND NOT IOS
    DEFINES
        QT_QMLTEST_DATADIR="${CMAKE_CURRENT_SOURCE_DIR}/data"
)
