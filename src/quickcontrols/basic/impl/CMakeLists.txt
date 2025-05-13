# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## qtquickcontrols2basicstyleimplplugin Plugin:
#####################################################################

qt_internal_add_qml_module(QuickControls2BasicStyleImpl
    URI "QtQuick.Controls.Basic.impl"
    VERSION "${PROJECT_VERSION}"
    CLASS_NAME QtQuickControls2BasicStyleImplPlugin
    PLUGIN_TARGET qtquickcontrols2basicstyleimplplugin
    DEPENDENCIES
        QtQuick/auto
    SOURCES
        qquickbasicbusyindicator.cpp qquickbasicbusyindicator_p.h
        qquickbasicdial.cpp qquickbasicdial_p.h
        qquickbasicprogressbar.cpp qquickbasicprogressbar_p.h
    DEFINES
        QT_NO_CAST_FROM_ASCII
        QT_NO_CAST_TO_ASCII
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::QmlPrivate
        Qt::QuickControls2ImplPrivate
        Qt::QuickPrivate
        Qt::QuickTemplates2Private
)
