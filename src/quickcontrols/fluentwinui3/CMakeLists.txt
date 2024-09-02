# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## qtquickcontrols2fluentwinui3styleplugin Plugin:
#####################################################################

set(qml_files
    "ApplicationWindow.qml"
    "BusyIndicator.qml"
    "Button.qml"
    "CheckBox.qml"
    "CheckDelegate.qml"
    "ComboBox.qml"
    "DelayButton.qml"
    "Dialog.qml"
    "DialogButtonBox.qml"
    "Frame.qml"
    "GroupBox.qml"
    "ItemDelegate.qml"
    "Menu.qml"
    "MenuBar.qml"
    "MenuBarItem.qml"
    "MenuItem.qml"
    "MenuSeparator.qml"
    "Popup.qml"
    "ProgressBar.qml"
    "PageIndicator.qml"
    "RadioButton.qml"
    "RadioDelegate.qml"
    "RangeSlider.qml"
    "RoundButton.qml"
    "Slider.qml"
    "SpinBox.qml"
    "SwipeDelegate.qml"
    "Switch.qml"
    "SwitchDelegate.qml"
    "TabBar.qml"
    "TabButton.qml"
    "TextField.qml"
    "TextArea.qml"
    "ToolBar.qml"
    "ToolButton.qml"
    "ToolSeparator.qml"
    "ToolTip.qml"
    "Config.qml"
    "StyleImage.qml"
    "FocusFrame.qml"
)

set_source_files_properties(Config.qml PROPERTIES
    QT_QML_SINGLETON_TYPE TRUE
)

file(GLOB light_theme_resources RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "light/images/*.png")
file(GLOB dark_theme_resources RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "dark/images/*.png")
file(GLOB icons_theme_resources RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "icons/*.png")

add_subdirectory(impl)

qt_internal_add_qml_module(qtquickcontrols2fluentwinui3styleplugin
    URI "QtQuick.Controls.FluentWinUI3"
    VERSION "${PROJECT_VERSION}"
    PAST_MAJOR_VERSIONS 2
    CLASS_NAME QtQuickControls2FluentWinUI3StylePlugin
    IMPORTS
        QtQuick.Controls.Fusion/auto
    PLUGIN_TARGET qtquickcontrols2fluentwinui3styleplugin
    NO_PLUGIN_OPTIONAL
    NO_GENERATE_PLUGIN_SOURCE
    SOURCES
        qtquickcontrols2fluentwinui3styleplugin.cpp
        qquickfluentwinui3theme_p.h qquickfluentwinui3theme.cpp
        qquickfluentwinui3focusframe_p.h qquickfluentwinui3focusframe.cpp
    QML_FILES
        ${qml_files}
    RESOURCES
        ${light_theme_resources}
        ${dark_theme_resources}
        ${icons_theme_resources}
    DEFINES
        QT_NO_CAST_FROM_ASCII
        QT_NO_CAST_TO_ASCII
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::QmlPrivate
        Qt::QuickControls2FluentWinUI3StyleImpl
        Qt::QuickControls2ImplPrivate
        Qt::QuickControls2Private
        Qt::QuickPrivate
        Qt::QuickTemplates2Private
)

_qt_internal_add_qml_static_plugin_dependency(qtquickcontrols2fluentwinui3styleplugin
                                              qtquickcontrols2fluentwinui3styleimplplugin)

# Fusion style is the required fallback style.
_qt_internal_add_qml_static_plugin_dependency(qtquickcontrols2fluentwinui3styleplugin
                                              qtquickcontrols2fusionstyleplugin)
