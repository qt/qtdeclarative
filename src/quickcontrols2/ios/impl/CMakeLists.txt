# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

#####################################################################
## qtquickcontrols2iosstyleimplplugin Plugin:
#####################################################################

set(qml_files
    "CursorDelegate.qml"
)

qt_internal_add_qml_module(qtquickcontrols2iosstyleimplplugin
    URI "QtQuick.Controls.iOS.impl"
    VERSION "${PROJECT_VERSION}"
    PLUGIN_TARGET qtquickcontrols2iosstyleimplplugin
    NO_PLUGIN_OPTIONAL
    SOURCES
        qquickioscursorflashtimer_p.h qquickioscursorflashtimer.cpp
    QML_FILES
        ${qml_files}
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::QmlPrivate
        Qt::QuickPrivate
)
