# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## qtquickcontrols2fluentwinui3styleimplplugin Plugin:
#####################################################################
set(qml_files
    "ButtonBackground.qml"
    "CheckIndicator.qml"
    "RadioIndicator.qml"
    "SwitchIndicator.qml"
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
    QML_FILES
        ${qml_files}
    DEFINES
        QT_NO_CAST_FROM_ASCII
        QT_NO_CAST_TO_ASCII
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::Qml
        Qt::QmlPrivate
        Qt::Quick
        Qt::QuickPrivate
)
