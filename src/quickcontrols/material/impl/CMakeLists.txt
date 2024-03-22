# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## qtquickcontrols2materialstyleimplplugin Plugin:
#####################################################################

set(qml_files
    "BoxShadow.qml"
    "CheckIndicator.qml"
    "CursorDelegate.qml"
    "ElevationEffect.qml"
    "RadioIndicator.qml"
    "RectangularGlow.qml"
    "RoundedElevationEffect.qml"
    "SliderHandle.qml"
    "SwitchIndicator.qml"
)

qt_internal_add_qml_module(QuickControls2MaterialStyleImpl
    URI "QtQuick.Controls.Material.impl"
    VERSION "${PROJECT_VERSION}"
    PAST_MAJOR_VERSIONS 2
    CLASS_NAME QtQuickControls2MaterialStyleImplPlugin
    DEPENDENCIES
        QtQuick/auto
        QtQuick.Controls.impl/auto
    PLUGIN_TARGET qtquickcontrols2materialstyleimplplugin
    SOURCES
        qquickmaterialbusyindicator.cpp qquickmaterialbusyindicator_p.h
        qquickmaterialplaceholdertext.cpp qquickmaterialplaceholdertext_p.h
        qquickmaterialprogressbar.cpp qquickmaterialprogressbar_p.h
        qquickmaterialripple.cpp qquickmaterialripple_p.h
        qquickmaterialtextcontainer.cpp qquickmaterialtextcontainer_p.h
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
        Qt::QuickControls2ImplPrivate
        Qt::Quick
        Qt::QuickPrivate
        Qt::QuickTemplates2Private
)
