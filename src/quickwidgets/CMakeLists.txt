# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from quickwidgets.pro.

#####################################################################
## QuickWidgets Module:
#####################################################################

qt_internal_add_module(QuickWidgets
    SOURCES
        qquickwidget.cpp qquickwidget.h qquickwidget_p.h
        qtquickwidgetsglobal.h
    DEFINES
        QT_NO_FOREACH
        QT_NO_INTEGER_EVENT_COORDINATES
        QT_NO_URL_CAST_FROM_STRING
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::QmlPrivate
        Qt::QuickPrivate
        Qt::WidgetsPrivate
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::Qml
        Qt::Quick
        Qt::Widgets
    PRIVATE_MODULE_INTERFACE
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::QmlPrivate
        Qt::QuickPrivate
        Qt::WidgetsPrivate
)

qt_internal_extend_target(QuickWidgets CONDITION QT_FEATURE_accessibility
    SOURCES
        qaccessiblequickwidget.cpp qaccessiblequickwidget_p.h
        qaccessiblequickwidgetfactory.cpp qaccessiblequickwidgetfactory_p.h
)
