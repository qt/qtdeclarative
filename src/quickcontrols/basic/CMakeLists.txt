# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## qtquickcontrols2basicstyleplugin Plugin:
#####################################################################

set(qml_files
    "AbstractButton.qml"
    "Action.qml"
    "ActionGroup.qml"
    "ApplicationWindow.qml"
    "BusyIndicator.qml"
    "Button.qml"
    "ButtonGroup.qml"
    "CheckBox.qml"
    "CheckDelegate.qml"
    "ComboBox.qml"
    "Container.qml"
    "Control.qml"
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
    "ScrollIndicator.qml"
    "ScrollView.qml"
    "SelectionRectangle.qml"
    "Slider.qml"
    "SpinBox.qml"
    "SplitView.qml"
    "StackView.qml"
    "SwipeDelegate.qml"
    "Switch.qml"
    "SwitchDelegate.qml"
    "SwipeView.qml"
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
if (QT_FEATURE_quicktemplates2_calendar)
    list(APPEND qml_files
        "Calendar.qml"
        "CalendarModel.qml"
        "DayOfWeekRow.qml"
        "MonthGrid.qml"
        "WeekNumberColumn.qml"
    )
endif()
if (QT_FEATURE_quick_treeview)
    list(APPEND qml_files
        "TreeViewDelegate.qml"
    )
endif()

set_source_files_properties(Action.qml PROPERTIES
    QT_QML_SOURCE_VERSIONS "2.3;6.0"
)
set_source_files_properties(ActionGroup.qml PROPERTIES
    QT_QML_SOURCE_VERSIONS "2.3;6.0"
)
if (QT_FEATURE_quicktemplates2_calendar)
    set_source_files_properties(Calendar.qml PROPERTIES
        QT_QML_SINGLETON_TYPE TRUE
        QT_QML_SOURCE_VERSIONS "6.3"
    )
    set_source_files_properties(CalendarModel.qml PROPERTIES
        QT_QML_SOURCE_VERSIONS "6.3"
    )
    set_source_files_properties(DayOfWeekRow.qml PROPERTIES
        QT_QML_SOURCE_VERSIONS "6.3"
    )
endif()
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
if (QT_FEATURE_quicktemplates2_calendar)
    set_source_files_properties(MonthGrid.qml PROPERTIES
        QT_QML_SOURCE_VERSIONS "6.3"
    )
endif()
set_source_files_properties(RoundButton.qml PROPERTIES
    QT_QML_SOURCE_VERSIONS "2.1;6.0"
)
set_source_files_properties(ScrollView.qml PROPERTIES
    QT_QML_SOURCE_VERSIONS "2.2;6.0"
)
set_source_files_properties(SelectionRectangle.qml PROPERTIES
    QT_QML_SOURCE_VERSIONS "6.2"
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
if (QT_FEATURE_quicktemplates2_calendar)
    set_source_files_properties(WeekNumberColumn.qml PROPERTIES
        QT_QML_SOURCE_VERSIONS "6.3"
    )
endif()

qt_internal_add_qml_module(qtquickcontrols2basicstyleplugin
    URI "QtQuick.Controls.Basic"
    VERSION "${PROJECT_VERSION}"
    PAST_MAJOR_VERSIONS 2
    CLASS_NAME QtQuickControls2BasicStylePlugin
    DEPENDENCIES
        QtQuick/auto
    IMPORTS
        QtQuick.Controls.impl/auto
    PLUGIN_TARGET qtquickcontrols2basicstyleplugin
    NO_PLUGIN_OPTIONAL
    NO_GENERATE_PLUGIN_SOURCE
    SOURCES
        qquickbasicstyle.cpp qquickbasicstyle_p.h
        qquickbasictheme.cpp qquickbasictheme_p.h
        qtquickcontrols2basicstyleplugin.cpp
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

# Resources:
set(qtquickcontrols2basicstyleplugin_resource_files
    "images/arrow-indicator.png"
    "images/arrow-indicator@2x.png"
    "images/arrow-indicator@3x.png"
    "images/arrow-indicator@4x.png"
    "images/check.png"
    "images/check@2x.png"
    "images/check@3x.png"
    "images/check@4x.png"
    "images/dial-indicator.png"
    "images/dial-indicator@2x.png"
    "images/dial-indicator@3x.png"
    "images/dial-indicator@4x.png"
    "images/double-arrow.png"
    "images/double-arrow@2x.png"
    "images/double-arrow@3x.png"
    "images/double-arrow@4x.png"
    "images/drop-indicator.png"
    "images/drop-indicator@2x.png"
    "images/drop-indicator@3x.png"
    "images/drop-indicator@4x.png"
)

qt_internal_add_resource(qtquickcontrols2basicstyleplugin "qtquickcontrols2basicstyleplugin"
    PREFIX
        "/qt-project.org/imports/QtQuick/Controls/Basic"
    FILES
        ${qtquickcontrols2basicstyleplugin_resource_files}
)

add_subdirectory(impl)

_qt_internal_add_qml_static_plugin_dependency(qtquickcontrols2basicstyleplugin
                                              qtquickcontrols2basicstyleimplplugin)
