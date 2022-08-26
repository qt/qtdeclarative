# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## qmldom Tool:
#####################################################################

qt_get_tool_target_name(target_name qmldom)
qt_internal_add_tool(${target_name}
    TARGET_DESCRIPTION "QML Dom handler"
    TOOLS_TARGET Qml # special case
    SOURCES
        qmldomtool.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::QmlDomPrivate
)
qt_internal_return_unless_building_tools()
