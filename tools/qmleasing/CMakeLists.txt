# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from qmleasing.pro.

#####################################################################
## qmleasing App:
#####################################################################

qt_internal_add_app(qmleasing
    SOURCES
        import.ui
        main.cpp
        mainwindow.cpp mainwindow.h
        pane.ui
        properties.ui
        segmentproperties.cpp segmentproperties.h
        splineeditor.cpp splineeditor.h
    LIBRARIES
        Qt::Gui
        Qt::Qml
        Qt::Quick
        Qt::Widgets
    ENABLE_AUTOGEN_TOOLS
        uic
)

set_target_properties(qmleasing PROPERTIES WIN32_EXECUTABLE FALSE)

# Resources:
set(resources_resource_files
    "Button.qml"
    "preview.qml"
)

qt_internal_add_resource(qmleasing "resources"
    PREFIX
        "/"
    FILES
        ${resources_resource_files}
)

set_target_properties(qmleasing PROPERTIES
    QT_TARGET_DESCRIPTION "QML Easing Curve Editor"
)
