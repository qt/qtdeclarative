# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## qqcstylegenerator App:
#####################################################################

qt_internal_add_app(qqcstylegenerator
    TARGET_DESCRIPTION "Quick Controls Style Generator"
    EXCEPTIONS
    SOURCES
        src/main.cpp
        src/stylegenerator.h
        src/jsontools.h src/jsontools.cpp
        src/bridge.h src/bridge.cpp
    LIBRARIES
        Qt::GuiPrivate
        Qt::Network
        Qt::Quick
        Qt::QuickControls2
)

qt_add_resources(qqcstylegenerator "controls"
    PREFIX "/"
    BASE templates
    FILES
        main.qml
        templates/Button.qml
        templates/CheckBox.qml
        templates/ComboBox.qml
        templates/Frame.qml
        templates/ItemDelegate.qml
        templates/Popup.qml
        templates/RadioButton.qml
        templates/RangeSlider.qml
        templates/ScrollBar.qml
        templates/ScrollIndicator.qml
        templates/Slider.qml
        templates/SpinBox.qml
        templates/Switch.qml
        templates/TabBar.qml
        templates/TabButton.qml
        templates/TextArea.qml
        templates/TextField.qml
        templates/ToolBar.qml
        templates/ToolButton.qml
        templates/impl/ComboBoxPopup.qml
        config.json
        figma_and_qt@2x.png
        background.png
)

target_include_directories(qqcstylegenerator PUBLIC src)
