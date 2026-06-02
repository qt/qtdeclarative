# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_get_tool_target_name(target_name qmltc)
qt_internal_add_tool(${target_name}
    TARGET_DESCRIPTION "QML type compiler"
    TOOLS_TARGET Qml
    SOURCES
        main.cpp
    LIBRARIES
        Qt::QmltcPrivate
)
qt_internal_return_unless_building_tools()
