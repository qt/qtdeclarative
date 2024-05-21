# Copyright (C) 2024 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## qtquickcontrols2fluentstyleplugin Plugin:
#####################################################################

set(qml_files
    "ApplicationWindow.qml"
    "Button.qml"
    "CheckBox.qml"
    "RadioButton.qml"
    "RangeSlider.qml"
    "Slider.qml"
    "Switch.qml"
    "Config.qml" # TODO: move to impl module
    "StyleImage.qml" # TODO: move to impl module
)

set_source_files_properties(Config.qml PROPERTIES
    QT_QML_SINGLETON_TYPE TRUE
)

file(GLOB light_theme_resources RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "light/images/*.png")
file(GLOB dark_theme_resources RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "dark/images/*.png")

# add_subdirectory(impl)

qt_internal_add_qml_module(qtquickcontrols2fluentstyleplugin
    URI "QtQuick.Controls.Fluent"
    VERSION "${PROJECT_VERSION}"
    PAST_MAJOR_VERSIONS 2
    CLASS_NAME QtQuickControls2FluentStylePlugin
    IMPORTS
        QtQuick.Controls.Fusion/auto
    PLUGIN_TARGET qtquickcontrols2fluentstyleplugin
    NO_PLUGIN_OPTIONAL
    NO_GENERATE_PLUGIN_SOURCE
    SOURCES
        qtquickcontrols2fluentstyleplugin.cpp
        qquickfluenttheme_p.h qquickfluenttheme.cpp
    QML_FILES
        ${qml_files}
    RESOURCES
        ${light_theme_resources}
        ${dark_theme_resources}
    DEFINES
        QT_NO_CAST_FROM_ASCII
        QT_NO_CAST_TO_ASCII
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::QmlPrivate
        # Qt::QuickControls2FluentStyleImpl
        Qt::QuickControls2ImplPrivate
        Qt::QuickControls2Private
        Qt::QuickPrivate
        Qt::QuickTemplates2Private
)

# _qt_internal_add_qml_static_plugin_dependency(qtquickcontrols2fluentstyleplugin
#                                               qtquickcontrols2fluentstyleimplplugin)

# Fusion style is the required fallback style.
_qt_internal_add_qml_static_plugin_dependency(qtquickcontrols2fluentstyleplugin
                                              qtquickcontrols2fusionstyleplugin)
