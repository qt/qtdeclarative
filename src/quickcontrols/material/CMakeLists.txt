# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## qtquickcontrols2materialstyleplugin Plugin:
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
    "SwipeView.qml"
    "Switch.qml"
    "SwitchDelegate.qml"
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

qt_internal_add_qml_module(qtquickcontrols2materialstyleplugin
    URI "QtQuick.Controls.Material"
    VERSION "${PROJECT_VERSION}"
    PAST_MAJOR_VERSIONS 2
    CLASS_NAME QtQuickControls2MaterialStylePlugin
    DEPENDENCIES
        QtQuick/auto
    IMPORTS
        QtQuick.Controls.Basic/auto
    PLUGIN_TARGET qtquickcontrols2materialstyleplugin
    NO_PLUGIN_OPTIONAL
    NO_GENERATE_PLUGIN_SOURCE
    SOURCES
        qquickmaterialstyle.cpp qquickmaterialstyle_p.h
        qquickmaterialtheme.cpp qquickmaterialtheme_p.h
        qtquickcontrols2materialstyleplugin.cpp
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

qt_internal_add_resource(qtquickcontrols2materialstyleplugin "qtquickcontrols2materialstyleplugin"
    PREFIX
        "/qt-project.org/imports/QtQuick/Controls/Material"
    FILES
        "images/arrow-indicator.png"
        "images/arrow-indicator@2x.png"
        "images/arrow-indicator@3x.png"
        "images/arrow-indicator@4x.png"
        "images/check.png"
        "images/check@2x.png"
        "images/check@3x.png"
        "images/check@4x.png"
        "images/drop-indicator.png"
        "images/drop-indicator@2x.png"
        "images/drop-indicator@3x.png"
        "images/drop-indicator@4x.png"
)

qt_internal_add_shaders(qtquickcontrols2materialstyleplugin "qtquickcontrols2materialstyleplugin_shaders"
    SILENT
    BATCHABLE
    PRECOMPILE
    OPTIMIZED
    PREFIX
        "/qt-project.org/imports/QtQuick/Controls/Material"
    FILES
        "shaders/RectangularGlow.frag"
)

add_subdirectory(impl)

_qt_internal_add_qml_static_plugin_dependency(qtquickcontrols2materialstyleplugin quickwindow)
_qt_internal_add_qml_static_plugin_dependency(qtquickcontrols2materialstyleplugin
                                              qtquickcontrols2materialstyleimplplugin)

# Basic style is the required fallback style.
_qt_internal_add_qml_static_plugin_dependency(qtquickcontrols2materialstyleplugin
                                              qtquickcontrols2basicstyleplugin)
