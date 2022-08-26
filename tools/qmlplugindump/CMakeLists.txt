# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from qmlplugindump.pro.

#####################################################################
## qmlplugindump Tool:
#####################################################################

qt_get_tool_target_name(target_name qmlplugindump)
qt_internal_add_tool(${target_name}
    TARGET_DESCRIPTION "QML Plugin Metadata Dumper"
    TOOLS_TARGET Qml # special case
    SOURCES
        main.cpp
        qmltypereader.cpp qmltypereader.h
    LIBRARIES
        Qt::QmlTypeRegistrarPrivate
        Qt::CorePrivate
        Qt::Gui
        Qt::Qml
        Qt::QmlPrivate
        Qt::QuickPrivate
)
qt_internal_return_unless_building_tools()

#### Keys ignored in scope 1:.:.:qmlplugindump.pro:<TRUE>:
# QMAKE_TARGET_DESCRIPTION = "QML" "Plugin" "Metadata" "Dumper"
# QTPLUGIN.platforms = "qminimal"

## Scopes:
#####################################################################

qt_internal_extend_target(${target_name} CONDITION TARGET Qt::Widgets
    PUBLIC_LIBRARIES
        Qt::Widgets
)

qt_internal_extend_target(${target_name} CONDITION MACOS
    LINK_OPTIONS
        "-Wl,-sectcreate,__TEXT,__info_plist,${CMAKE_CURRENT_SOURCE_DIR}/Info.plist" # special case
)
