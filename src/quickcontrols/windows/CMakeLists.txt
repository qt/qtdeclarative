# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## qtquickcontrols2windowsstyleplugin Plugin:
#####################################################################

set(qml_files
    "ApplicationWindow.qml"
    "Button.qml"
    "CheckBox.qml"
    "ComboBox.qml"
    "Frame.qml"
    "GroupBox.qml"
    "ProgressBar.qml"
    "RadioButton.qml"
    "SelectionRectangle.qml"
    "Slider.qml"
    "SpinBox.qml"
    "TextArea.qml"
    "TextField.qml"
    "ScrollBar.qml"
    "ScrollView.qml"
)

qt_internal_add_qml_module(qtquickcontrols2windowsstyleplugin
    URI "QtQuick.Controls.Windows"
    VERSION "${PROJECT_VERSION}"
    CLASS_NAME QtQuickControls2WindowsStylePlugin
    IMPORTS
        QtQuick.Controls.Fusion/auto
    PAST_MAJOR_VERSIONS 2
    PLUGIN_TARGET qtquickcontrols2windowsstyleplugin
    NO_PLUGIN_OPTIONAL
    NO_GENERATE_PLUGIN_SOURCE
    SOURCES
        qtquickcontrols2windowsstyleplugin.cpp
    QML_FILES
        ${qml_files}
    DEFINES
        QT_NO_CAST_FROM_ASCII
        QT_NO_CAST_TO_ASCII
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::QmlPrivate
        Qt::QuickControls2Private
        Qt::QuickPrivate
        Qt::QuickTemplates2Private
)

# Native style is a dependency of the Windows style.
_qt_internal_add_qml_static_plugin_dependency(qtquickcontrols2windowsstyleplugin
                                              qtquickcontrols2nativestyleplugin)
# Fusion style is the required fallback style.
_qt_internal_add_qml_static_plugin_dependency(qtquickcontrols2windowsstyleplugin
                                              qtquickcontrols2fusionstyleplugin)
