# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## qtquickcontrols2iosstyleimplplugin Plugin:
#####################################################################

set(qml_files
    "CursorDelegate.qml"
    "DialogButtonBoxDelegate.qml"
)

qt_internal_add_qml_module(QuickControls2IOSStyleImpl
    URI "QtQuick.Controls.iOS.impl"
    VERSION "${PROJECT_VERSION}"
    PLUGIN_TARGET qtquickcontrols2iosstyleimplplugin
    SOURCES
        qquickiosstyle.cpp qquickiosstyle_p.h
        qquickioscursorflashtimer_p.h qquickioscursorflashtimer.cpp
    QML_FILES
        ${qml_files}
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::QmlPrivate
        Qt::QuickPrivate
)
