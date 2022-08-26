# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from qjstest.pro.

#####################################################################
## qjstest Tool:
#####################################################################

qt_get_tool_target_name(target_name qjstest)
qt_internal_add_tool(${target_name}
    TARGET_DESCRIPTION "Javascript test runner"
    SOURCES
        main.cpp
        test262runner.cpp test262runner.h
    DEFINES
        QT_DEPRECATED_WARNINGS
    INCLUDE_DIRECTORIES
        .
    LIBRARIES
        Qt::Gui
        Qt::QmlPrivate
)
qt_internal_return_unless_building_tools()

#### Keys ignored in scope 1:.:.:qjstest.pro:<TRUE>:
# QMAKE_TARGET_DESCRIPTION = "Javascript" "test" "runner"
# TEMPLATE = "app"
