# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## qtquickcontrols2universalstyleimplplugin Plugin:
#####################################################################

set(qml_files
    "CheckIndicator.qml"
    "CopyAction.qml"
    "CutAction.qml"
    "DeleteAction.qml"
    "PasteAction.qml"
    "RadioIndicator.qml"
    "RedoAction.qml"
    "SelectAllAction.qml"
    "SwitchIndicator.qml"
    "TextEditingContextMenu.qml"
    "UndoAction.qml"
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

qt_internal_add_qml_module(QuickControls2UniversalStyleImpl
    URI "QtQuick.Controls.Universal.impl"
    VERSION "${PROJECT_VERSION}"
    PAST_MAJOR_VERSIONS 2
    CLASS_NAME QtQuickControls2UniversalStyleImplPlugin
    DEPENDENCIES
        QtQuick/auto
    PLUGIN_TARGET qtquickcontrols2universalstyleimplplugin
    SOURCES
        qquickuniversalbusyindicator.cpp qquickuniversalbusyindicator_p.h
        qquickuniversalfocusrectangle.cpp qquickuniversalfocusrectangle_p.h
        qquickuniversalprogressbar.cpp qquickuniversalprogressbar_p.h
    QML_FILES
        ${qml_files}
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
