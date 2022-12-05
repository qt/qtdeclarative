# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## qtquickcontrols2imaginestyleplugin Plugin:
#####################################################################

set(qml_files
    "ApplicationWindow.qml"
    "BusyIndicator.qml"
    "Button.qml"
    "CheckBox.qml"
    "CheckDelegate.qml"
    "ComboBox.qml"
    "DelayButton.qml"
    "Dial.qml"
    "Dialog.qml"
    "DialogButtonBox.qml"
    "Drawer.qml"
    "Frame.qml"
    "GroupBox.qml"
    "HorizontalHeaderView.qml"
    "ItemDelegate.qml"
    "Label.qml"
    "Menu.qml"
    "MenuItem.qml"
    "MenuSeparator.qml"
    "PageIndicator.qml"
    "Page.qml"
    "Pane.qml"
    "Popup.qml"
    "ProgressBar.qml"
    "RadioButton.qml"
    "RadioDelegate.qml"
    "RangeSlider.qml"
    "RoundButton.qml"
    "ScrollView.qml"
    "ScrollBar.qml"
    "ScrollIndicator.qml"
    "SelectionRectangle.qml"
    "Slider.qml"
    "SpinBox.qml"
    "SplitView.qml"
    "StackView.qml"
    "SwipeDelegate.qml"
    "SwipeView.qml"
    "Switch.qml"
    "SwitchDelegate.qml"
    "TextField.qml"
    "TextArea.qml"
    "TabBar.qml"
    "TabButton.qml"
    "ToolBar.qml"
    "ToolButton.qml"
    "ToolSeparator.qml"
    "ToolTip.qml"
    "Tumbler.qml"
    "VerticalHeaderView.qml"
)
set_source_files_properties(DelayButton.qml PROPERTIES
    QT_QML_SOURCE_VERSIONS "2.2;6.0"
)
set_source_files_properties(Dialog.qml PROPERTIES
    QT_QML_SOURCE_VERSIONS "2.1;6.0"
)
set_source_files_properties(DialogButtonBox.qml PROPERTIES
    QT_QML_SOURCE_VERSIONS "2.1;6.0"
)
set_source_files_properties(HorizontalHeaderView.qml PROPERTIES
    QT_QML_SOURCE_VERSIONS "2.15;6.0"
)
set_source_files_properties(MenuSeparator.qml PROPERTIES
    QT_QML_SOURCE_VERSIONS "2.1;6.0"
)
set_source_files_properties(RoundButton.qml PROPERTIES
    QT_QML_SOURCE_VERSIONS "2.1;6.0"
)
set_source_files_properties(SplitView.qml PROPERTIES
    QT_QML_SOURCE_VERSIONS "2.13;6.0"
)
set_source_files_properties(ToolSeparator.qml PROPERTIES
    QT_QML_SOURCE_VERSIONS "2.1;6.0"
)
set_source_files_properties(VerticalHeaderView.qml PROPERTIES
    QT_QML_SOURCE_VERSIONS "2.15;6.0"
)

qt_internal_add_qml_module(qtquickcontrols2imaginestyleplugin
    URI "QtQuick.Controls.Imagine"
    VERSION "${PROJECT_VERSION}"
    PAST_MAJOR_VERSIONS 2
    CLASS_NAME QtQuickControls2ImagineStylePlugin
    IMPORTS
        QtQuick.Controls.Basic/auto
    PLUGIN_TARGET qtquickcontrols2imaginestyleplugin
    NO_PLUGIN_OPTIONAL
    NO_GENERATE_PLUGIN_SOURCE
    SOURCES
        qquickimaginestyle.cpp qquickimaginestyle_p.h
        qquickimaginetheme.cpp qquickimaginetheme_p.h
        qtquickcontrols2imaginestyleplugin.cpp
    QML_FILES
        ${qml_files}
    DEFINES
        QT_NO_CAST_FROM_ASCII
        QT_NO_CAST_TO_ASCII
    LIBRARIES
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::QmlPrivate
        Qt::QuickControls2ImplPrivate
        Qt::QuickControls2Private
        Qt::QuickPrivate
        Qt::QuickTemplates2Private
)

file(GLOB resource_glob_0 RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "images/*.png")
foreach(file IN LISTS resource_glob_0)
    set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/${file}" PROPERTIES QT_RESOURCE_ALIAS "${file}")
endforeach()

file(GLOB resource_glob_1 RELATIVE "${CMAKE_CURRENT_SOURCE_DIR}" "images/*.webp")
foreach(file IN LISTS resource_glob_1)
    set_source_files_properties("${CMAKE_CURRENT_SOURCE_DIR}/${file}" PROPERTIES QT_RESOURCE_ALIAS "${file}")
endforeach()

# Resources:
set(qmake_qtquickcontrols2imaginestyleplugin_resource_files
    ${resource_glob_0}
    ${resource_glob_1}
)

qt_internal_add_resource(qtquickcontrols2imaginestyleplugin "qmake_qtquickcontrols2imaginestyleplugin"
    PREFIX
        "/qt-project.org/imports/QtQuick/Controls/Imagine"
    FILES
        ${qmake_qtquickcontrols2imaginestyleplugin_resource_files}
)

add_subdirectory(impl)

_qt_internal_add_qml_static_plugin_dependency(qtquickcontrols2imaginestyleplugin quickwindow)
_qt_internal_add_qml_static_plugin_dependency(qtquickcontrols2imaginestyleplugin
                                              qtquickcontrols2imaginestyleimplplugin)

# Basic style is the required fallback style.
_qt_internal_add_qml_static_plugin_dependency(qtquickcontrols2imaginestyleplugin
                                              qtquickcontrols2basicstyleplugin)
