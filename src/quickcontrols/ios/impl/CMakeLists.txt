# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## qtquickcontrols2iosstyleimplplugin Plugin:
#####################################################################

set(qml_files
    "CopyAction.qml"
    "CursorDelegate.qml"
    "CutAction.qml"
    "DeleteAction.qml"
    "DialogButtonBoxDelegate.qml"
    "PasteAction.qml"
    "SelectAllAction.qml"
    "TextEditingContextMenu.qml"
)

set_source_files_properties(
    CopyAction.qml
    CutAction.qml
    DeleteAction.qml
    PasteAction.qml
    SelectAllAction.qml
    TextEditingContextMenu.qml
    PROPERTIES
        QT_QML_SOURCE_VERSIONS "6.11"
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
