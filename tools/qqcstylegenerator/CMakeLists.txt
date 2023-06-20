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
    LIBRARIES
        Qt::GuiPrivate
        Qt::Network
)

qt_add_resources(qqcstylegenerator "controls"
    PREFIX "/"
    BASE templates/controls
    FILES
        templates/Button.qml
        templates/CheckBox.qml
        templates/Frame.qml
        templates/RadioButton.qml
        templates/RangeSlider.qml
        templates/Slider.qml
        templates/SpinBox.qml
        templates/Switch.qml
        templates/TabBar.qml
        templates/TabButton.qml
        templates/TextArea.qml
        templates/TextField.qml
        templates/ToolBar.qml
        templates/ToolButton.qml
        config.json
)
