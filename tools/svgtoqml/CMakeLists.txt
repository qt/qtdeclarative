# Copyright (C) 2023 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## svgtoqml Tool:
#####################################################################

qt_get_tool_target_name(target_name svgtoqml)
qt_internal_add_tool(${target_name}
    TARGET_DESCRIPTION "SVG to QML Converter"
    TOOLS_TARGET Quick
    EXTRA_CMAKE_FILES
        "${CMAKE_CURRENT_SOURCE_DIR}/${QT_CMAKE_EXPORT_NAMESPACE}SvgToQmlMacros.cmake"
    EXTRA_CMAKE_INCLUDES
        "${QT_CMAKE_EXPORT_NAMESPACE}SvgToQmlMacros.cmake"
    SOURCES
        main.cpp
    LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::GuiPrivate
        Qt::Qml
        Qt::Quick
        Qt::QuickVectorImagePrivate
        Qt::QuickVectorImageGeneratorPrivate
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
