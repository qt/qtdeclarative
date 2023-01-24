# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from qmllint.pro.

#####################################################################
## qmllint Tool:
#####################################################################

qt_get_tool_target_name(target_name qmllint)
qt_internal_add_tool(${target_name}
    TARGET_DESCRIPTION "QML Syntax Verifier"
    TOOLS_TARGET Qml # special case
    SOURCES
        main.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::QmlCompilerPrivate
        Qt::QmlToolingSettingsPrivate
)
qt_internal_return_unless_building_tools()

#### Keys ignored in scope 1:.:.:qmllint.pro:<TRUE>:
# QMAKE_TARGET_DESCRIPTION = "QML" "Syntax" "Verifier"
# _OPTION = "host_build"
