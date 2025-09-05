# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## qtquickcontrols2fusionstyleimplplugin Plugin:
#####################################################################

set(qml_files
    "ButtonPanel.qml"
    "CheckIndicator.qml"
    "RadioIndicator.qml"
    "SliderGroove.qml"
    "SliderHandle.qml"
    "SwitchIndicator.qml"
    "TextFieldBackground.qml"
)

qt_internal_add_qml_module(QuickControls2FusionStyleImpl
    URI "QtQuick.Controls.Fusion.impl"
    VERSION "${PROJECT_VERSION}"
    PAST_MAJOR_VERSIONS 2
    CLASS_NAME QtQuickControls2FusionStyleImplPlugin
    DEPENDENCIES
        QtQuick/auto
    PLUGIN_TARGET qtquickcontrols2fusionstyleimplplugin
    SOURCES
        qquickfusionbusyindicator.cpp qquickfusionbusyindicator_p.h
        qquickfusiondial.cpp qquickfusiondial_p.h
        qquickfusionknob.cpp qquickfusionknob_p.h
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
