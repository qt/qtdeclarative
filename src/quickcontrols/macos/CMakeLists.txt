# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## qtquickcontrols2macosstyleplugin Plugin:
#####################################################################

add_subdirectory(impl)

set(qml_files
    "BusyIndicator.qml"
    "Button.qml"
    "CheckBox.qml"
    "CheckDelegate.qml"
    "ComboBox.qml"
    "DelayButton.qml"
    "Dial.qml"
    "Dialog.qml"
    "DialogButtonBox.qml"
    "Frame.qml"
    "GroupBox.qml"
    "ItemDelegate.qml"
    "ProgressBar.qml"
    "RadioButton.qml"
    "RadioDelegate.qml"
    "RangeSlider.qml"
    "ScrollBar.qml"
    "ScrollView.qml"
    "SelectionRectangle.qml"
    "Slider.qml"
    "SpinBox.qml"
    "Switch.qml"
    "SwitchDelegate.qml"
    "TextArea.qml"
    "TextField.qml"
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
    RESOURCES
        images/busyindicator-dark.webp
        images/busyindicator-dark@2x.webp
        images/busyindicator-dark@3x.webp
        images/busyindicator-light.webp
        images/busyindicator-light@2x.webp
        images/busyindicator-light@3x.webp
    DEFINES
        QT_NO_CAST_FROM_ASCII
        QT_NO_CAST_TO_ASCII
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::QmlPrivate
        Qt::QuickControls2MacOSStyleImpl
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
