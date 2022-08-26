# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## qtquickcontrols2macosstyleplugin Plugin:
#####################################################################

set(qml_files
    "Button.qml"
    "Slider.qml"
    "GroupBox.qml"
    "CheckBox.qml"
    "RadioButton.qml"
    "SelectionRectangle.qml"
    "SpinBox.qml"
    "TextField.qml"
    "Frame.qml"
    "TextArea.qml"
    "ComboBox.qml"
    "ScrollView.qml"
    "ScrollBar.qml"
    "ProgressBar.qml"
    "Dial.qml"
    "Dialog.qml"
    "DialogButtonBox.qml"
)

if (QT_FEATURE_quick_treeview)
    list(APPEND qml_files
        "TreeViewDelegate.qml"
    )
endif()

qt_internal_add_qml_module(qtquickcontrols2macosstyleplugin
    URI "QtQuick.Controls.macOS"
    VERSION "${PROJECT_VERSION}"
    CLASS_NAME QtQuickControls2MacOSStylePlugin
    IMPORTS
        QtQuick.Controls.Fusion/auto
    PAST_MAJOR_VERSIONS 2
    PLUGIN_TARGET qtquickcontrols2macosstyleplugin
    NO_PLUGIN_OPTIONAL
    NO_GENERATE_PLUGIN_SOURCE
    SOURCES
        qtquickcontrols2macosstyleplugin.cpp
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

# Native style is a dependency of the macOS style.
_qt_internal_add_qml_static_plugin_dependency(qtquickcontrols2macosstyleplugin
                                              qtquickcontrols2nativestyleplugin)

# Fusion style is the required fallback style.
_qt_internal_add_qml_static_plugin_dependency(qtquickcontrols2macosstyleplugin
                                              qtquickcontrols2fusionstyleplugin)
