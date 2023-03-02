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
        src/qtbridgereader.h
        src/stylegenerator.h
        src/jsontools.h src/jsontools.cpp
    LIBRARIES
        Qt::GuiPrivate
)

qt_add_resources(qqcstylegenerator "controls"
    PREFIX "/"
    BASE templates/controls
    FILES
        templates/Button.qml
        templates/CheckBox.qml
        templates/Switch.qml
)
