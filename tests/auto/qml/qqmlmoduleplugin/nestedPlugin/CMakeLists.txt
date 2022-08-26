# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from nestedPlugin.pro.

#####################################################################
## nestedPlugin Generic Library:
#####################################################################

qt_internal_add_cmake_library(nestedPlugin
    MODULE # special case
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../imports/org/qtproject/AutoTestQmlNestedPluginType"
    SOURCES
        nestedPlugin.cpp
    LIBRARIES
        Qt::Core
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::Qml
        Qt::QmlPrivate
)

#### Keys ignored in scope 1:.:.:nestedPlugin.pro:<TRUE>:
# IMPORT_FILES = "qmldir"
# TEMPLATE = "lib"

# special case begin
qt_autogen_tools_initial_setup(nestedPlugin)
file(COPY qmldir
    DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/../imports/org/qtproject/AutoTestQmlNestedPluginType"
)
# special case end TEMPLATE = "lib"
