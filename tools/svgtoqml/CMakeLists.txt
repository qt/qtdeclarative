# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## svgtoqml Tool:
#####################################################################

qt_get_tool_target_name(target_name svgtoqml)
qt_internal_add_tool(${target_name}
    TARGET_DESCRIPTION "SVG to QML Converter"
    TOOLS_TARGET Quick
    SOURCES
        main.cpp
        qsvgloader.cpp qsvgloader_p.h
    LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::Qml
        Qt::Quick
        Qt::QuickPrivate
        Qt::Svg
        Qt::SvgPrivate
        Qt::QuickShapesPrivate
)
qt_internal_return_unless_building_tools()

set(resource_files
    "main.qml"
)

qt_internal_add_resource(${target_name} "qml"
    PREFIX
        "/"
    FILES
        ${resource_files}
)
