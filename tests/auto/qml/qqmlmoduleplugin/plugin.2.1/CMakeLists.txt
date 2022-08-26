# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from plugin.2.1.pro.

#####################################################################
## plugin.2.1 Generic Library:
#####################################################################

qt_internal_add_cmake_library(plugin.2.1
    MODULE
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../imports/org/qtproject/AutoTestQmlPluginType.2.1"
    SOURCES
        plugin.cpp
    LIBRARIES
        Qt::Core
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::Qml
        Qt::QmlPrivate
)

#### Keys ignored in scope 1:.:.:plugin.2.1.pro:<TRUE>:
# IMPORT_FILES = "qmldir"
# TEMPLATE = "lib"

qt_autogen_tools_initial_setup(plugin.2.1)
# special case begin
set_target_properties(plugin.2.1 PROPERTIES OUTPUT_NAME "plugin")
file(COPY qmldir
    DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/../imports/org/qtproject/AutoTestQmlPluginType.2.1"
)
# special case end
