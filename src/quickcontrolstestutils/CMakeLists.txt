# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

qt_internal_add_module(QuickControlsTestUtilsPrivate
    CONFIG_MODULE_NAME quickcontrolstestutilsprivate
    STATIC
    INTERNAL_MODULE
    SOURCES
        controlstestutils.cpp
        controlstestutils_p.h
        dialogstestutils.cpp
        dialogstestutils_p.h
        qtest_quickcontrols_p.h
    DEFINES
        QT_BUILD_SHARED_QUICK_CONTROLS_TEST_UTILS_LIB
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Test
        Qt::Qml
        Qt::QmlPrivate
        Qt::Quick
        Qt::QuickControls2
        Qt::QuickDialogs2QuickImplPrivate
        Qt::QuickPrivate
        Qt::QuickTemplates2
        Qt::QuickTemplates2Private
        Qt::QuickTestUtilsPrivate
)
