# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## qtquickcontrols2fluentwinui3styleimplplugin Plugin:
#####################################################################
set(qml_files
    "ButtonBackground.qml"
    "CheckIndicator.qml"
    "CopyAction.qml"
    "CutAction.qml"
    "DeleteAction.qml"
    "FocusFrame.qml"
    "PasteAction.qml"
    "RadioIndicator.qml"
    "RedoAction.qml"
    "SelectAllAction.qml"
    "SwitchIndicator.qml"
    "StyleImage.qml"
    "TextEditingContextMenu.qml"
    "UndoAction.qml"
)

set_source_files_properties(
    CopyAction.qml
    CutAction.qml
    DeleteAction.qml
    PasteAction.qml
    RedoAction.qml
    SelectAllAction.qml
    TextEditingContextMenu.qml
    UndoAction.qml
    PROPERTIES
        QT_QML_SOURCE_VERSIONS "6.11"
)

qt_internal_add_qml_module(QuickControls2FluentWinUI3StyleImpl
    URI "QtQuick.Controls.FluentWinUI3.impl"
    VERSION "${PROJECT_VERSION}"
    PAST_MAJOR_VERSIONS 2
    CLASS_NAME QtQuickControls2FluentWinUI3StyleImplPlugin
    DEPENDENCIES
        QtQuick/auto
    PLUGIN_TARGET qtquickcontrols2fluentwinui3styleimplplugin
    SOURCES
        qquickfluentwinui3focusstroke_p.h qquickfluentwinui3focusstroke.cpp
        qquickfluentwinui3focusframe_p.h qquickfluentwinui3focusframe.cpp
        qquickfluentwinui3styleimplglobal_p.h
    QML_FILES
        ${qml_files}
    DEFINES
        QT_BUILD_QUICKCONTROLS2FLUENTWINUI3STYLEIMPL_LIB
        QT_NO_CAST_FROM_ASCII
        QT_NO_CAST_TO_ASCII
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::Qml
        Qt::QmlPrivate
        Qt::Quick
        Qt::QuickPrivate
        Qt::QuickTemplates2Private
)
