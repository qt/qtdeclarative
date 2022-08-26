# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from qmltime.pro.

#####################################################################
## qmltime Tool:
#####################################################################

qt_get_tool_target_name(target_name qmltime)
qt_internal_add_tool(${target_name}
    TARGET_DESCRIPTION "QML Time"
    TOOLS_TARGET Qml # special case
    SOURCES
        qmltime.cpp qmltime.h
    LIBRARIES
        Qt::Gui
        Qt::Qml
        Qt::Quick
        Qt::QuickPrivate
)
qt_internal_return_unless_building_tools()

# Turn the tool into its own self-contained qml module
qt_internal_add_qml_module(${target_name}
    URI QmlTime
    VERSION 1.0
    NO_PLUGIN
)
