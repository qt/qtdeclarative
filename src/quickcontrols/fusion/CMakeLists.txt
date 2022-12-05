# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## qtquickcontrols2fusionstyleplugin Plugin:
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
    "MenuBar.qml"
    "MenuBarItem.qml"
    "MenuItem.qml"
    "MenuSeparator.qml"
    "Page.qml"
    "PageIndicator.qml"
    "Pane.qml"
    "Popup.qml"
    "ProgressBar.qml"
    "RadioButton.qml"
    "RadioDelegate.qml"
    "RangeSlider.qml"
    "RoundButton.qml"
    "ScrollBar.qml"
    "ScrollView.qml"
    "ScrollIndicator.qml"
    "SelectionRectangle.qml"
    "Slider.qml"
    "SpinBox.qml"
    "SplitView.qml"
    "SwipeDelegate.qml"
    "SwitchDelegate.qml"
    "Switch.qml"
    "TabBar.qml"
    "TabButton.qml"
    "TextArea.qml"
    "TextField.qml"
    "ToolBar.qml"
    "ToolButton.qml"
    "ToolSeparator.qml"
    "ToolTip.qml"
    "TreeViewDelegate.qml"
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
set_source_files_properties(MenuBar.qml PROPERTIES
    QT_QML_SOURCE_VERSIONS "2.3;6.0"
)
set_source_files_properties(MenuBarItem.qml PROPERTIES
    QT_QML_SOURCE_VERSIONS "2.3;6.0"
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

qt_internal_add_qml_module(qtquickcontrols2fusionstyleplugin
    URI "QtQuick.Controls.Fusion"
    VERSION "${PROJECT_VERSION}"
    PAST_MAJOR_VERSIONS 2
    CLASS_NAME QtQuickControls2FusionStylePlugin
    DEPENDENCIES
        QtQuick/auto
    IMPORTS
        QtQuick.Controls.Basic/auto
    PLUGIN_TARGET qtquickcontrols2fusionstyleplugin
    NO_PLUGIN_OPTIONAL
    NO_GENERATE_PLUGIN_SOURCE
    SOURCES
        qquickfusionstyle.cpp qquickfusionstyle_p.h
        qquickfusiontheme.cpp qquickfusiontheme_p.h
        qtquickcontrols2fusionstyleplugin.cpp
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

qt_internal_add_resource(qtquickcontrols2fusionstyleplugin "qtquickcontrols2fusionstyle"
    PREFIX
        "/qt-project.org/imports/QtQuick/Controls/Fusion"
    FILES
        "images/arrow.png"
        "images/arrow@2x.png"
        "images/arrow@3x.png"
        "images/arrow@4x.png"
        "images/checkmark.png"
        "images/checkmark@2x.png"
        "images/checkmark@3x.png"
        "images/checkmark@4x.png"
        "images/progressmask.png"
        "images/progressmask@2x.png"
        "images/progressmask@3x.png"
        "images/progressmask@4x.png"
)

add_subdirectory(impl)

_qt_internal_add_qml_static_plugin_dependency(qtquickcontrols2fusionstyleplugin quickwindow)
_qt_internal_add_qml_static_plugin_dependency(qtquickcontrols2fusionstyleplugin
                                              qtquickcontrols2fusionstyleimplplugin)

# Basic style is the required fallback style.
_qt_internal_add_qml_static_plugin_dependency(qtquickcontrols2fusionstyleplugin
                                              qtquickcontrols2basicstyleplugin)
