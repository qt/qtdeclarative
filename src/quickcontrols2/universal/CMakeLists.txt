# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## qtquickcontrols2universalstyleplugin Plugin:
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
    "ScrollView.qml"
    "ScrollBar.qml"
    "ScrollIndicator.qml"
    "SelectionRectangle.qml"
    "Slider.qml"
    "SpinBox.qml"
    "SplitView.qml"
    "StackView.qml"
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

qt_internal_add_qml_module(qtquickcontrols2universalstyleplugin
    URI "QtQuick.Controls.Universal"
    VERSION "${PROJECT_VERSION}"
    PAST_MAJOR_VERSIONS 2
    CLASS_NAME QtQuickControls2UniversalStylePlugin
    DEPENDENCIES
        QtQuick/auto
    IMPORTS
        QtQuick.Controls.Basic/auto
    PLUGIN_TARGET qtquickcontrols2universalstyleplugin
    NO_PLUGIN_OPTIONAL
    NO_GENERATE_PLUGIN_SOURCE
    SOURCES
        qquickuniversalstyle.cpp qquickuniversalstyle_p.h
        qquickuniversaltheme.cpp qquickuniversaltheme_p.h
        qtquickcontrols2universalstyleplugin.cpp
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

qt_internal_add_resource(qtquickcontrols2universalstyleplugin "qtquickcontrols2universalstyleplugin"
    PREFIX
        "/qt-project.org/imports/QtQuick/Controls/Universal"
    FILES
        "images/checkmark.png"
        "images/checkmark@2x.png"
        "images/checkmark@3x.png"
        "images/checkmark@4x.png"
        "images/downarrow.png"
        "images/downarrow@2x.png"
        "images/downarrow@3x.png"
        "images/downarrow@4x.png"
        "images/leftarrow.png"
        "images/leftarrow@2x.png"
        "images/leftarrow@3x.png"
        "images/leftarrow@4x.png"
        "images/rightarrow.png"
        "images/rightarrow@2x.png"
        "images/rightarrow@3x.png"
        "images/rightarrow@4x.png"
)

add_subdirectory(impl)

_qt_internal_add_qml_static_plugin_dependency(qtquickcontrols2universalstyleplugin quickwindow)
_qt_internal_add_qml_static_plugin_dependency(qtquickcontrols2universalstyleplugin
                                              qtquickcontrols2universalstyleimplplugin)

# Basic style is the required fallback style.
_qt_internal_add_qml_static_plugin_dependency(qtquickcontrols2universalstyleplugin
                                              qtquickcontrols2basicstyleplugin)
