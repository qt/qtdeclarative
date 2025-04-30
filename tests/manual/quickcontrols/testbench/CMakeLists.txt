# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from testbench.pro.

if (NOT QT_BUILD_STANDALONE_TESTS AND NOT QT_BUILDING_QT)
    cmake_minimum_required(VERSION 3.16)
    project(testbench LANGUAGES C CXX)
    find_package(Qt6BuildInternals COMPONENTS STANDALONE_TEST)
endif()

#####################################################################
## testbench Binary:
#####################################################################

qt_internal_add_manual_test(testbench
    GUI
    SOURCES
        assetfixer.cpp assetfixer.h
        clipboard.cpp clipboard.h
        directoryvalidator.cpp directoryvalidator.h
        main.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::Qml
        Qt::Quick
        Qt::QuickControls2
        Qt::QuickControls2Private
)

# Resources:
set(qml_resource_files
    "+Imagine/ApplicationWindow.qml"
    "+Imagine/ContentPane.qml"
    "+Imagine/Dialog.qml"
    "+Imagine/Menu.qml"
    "+Imagine/ToolBar.qml"
    "ApplicationWindow.qml"
    "ColorEditor.qml"
    "ContentPane.qml"
    "ControlContainer.qml"
    "Dialog.qml"
    "ExampleContainer.qml"
    "Menu.qml"
    "SettingsDialog.qml"
    "ToolBar.qml"
    "controls/BusyIndicator.qml"
    "controls/Button.qml"
    "controls/CheckBox.qml"
    "controls/CheckDelegate.qml"
    "controls/ComboBox.qml"
    "controls/DelayButton.qml"
    "controls/Dial.qml"
    "controls/Dialog.qml"
    "controls/Frame.qml"
    "controls/GroupBox.qml"
    "controls/ItemDelegate.qml"
    "controls/Label.qml"
    "controls/Menu.qml"
    "controls/MenuBar.qml"
    "controls/Page.qml"
    "controls/PageIndicator.qml"
    "controls/Pane.qml"
    "controls/ProgressBar.qml"
    "controls/RadioButton.qml"
    "controls/RadioDelegate.qml"
    "controls/RangeSlider.qml"
    "controls/RoundButton.qml"
    "controls/ScrollBar.qml"
    "controls/ScrollIndicator.qml"
    "controls/Slider.qml"
    "controls/SpinBox.qml"
    "controls/SplitView.qml"
    "controls/SwipeDelegate.qml"
    "controls/Switch.qml"
    "controls/SwitchDelegate.qml"
    "controls/TabBar.qml"
    "controls/TextArea.qml"
    "controls/TextField.qml"
    "controls/ToolBar.qml"
    "controls/ToolTip.qml"
    "controls/Tumbler.qml"
    "qtquickcontrols2.conf"
    "testbench.qml"
)

qt_internal_add_resource(testbench "qml"
    PREFIX
        "/"
    FILES
        ${qml_resource_files}
)
set(fonts_resource_files
    "fonts/fontello.ttf"
)

qt_internal_add_resource(testbench "fonts"
    PREFIX
        "/"
    FILES
        ${fonts_resource_files}
)


#### Keys ignored in scope 1:.:.:testbench.pro:<TRUE>:
# QML_IMPORT_PATH = <EMPTY>
# TEMPLATE = "app"

## Scopes:
#####################################################################

#### Keys ignored in scope 3:.:.:deployment.pri:UNIX AND NOT ANDROID:
# INSTALLS = "target"

#### Keys ignored in scope 5:.:.:deployment.pri:QNX:
# target.path = "/tmp/$${TARGET}/bin"

#### Keys ignored in scope 6:.:.:deployment.pri:else:
# target.path = "/opt/$${TARGET}/bin"
