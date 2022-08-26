# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from childplugin.pro.

#####################################################################
## childplugin Generic Library:
#####################################################################

qt_internal_add_cmake_library(childplugin
    MODULE # special case
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../../imports/org/qtproject/AutoTestQmlPluginType/ChildPlugin"
    SOURCES
        childplugin.cpp
    LIBRARIES
        Qt::Core
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::Qml
        Qt::QmlPrivate
)

#### Keys ignored in scope 1:.:.:childplugin.pro:<TRUE>:
# IMPORT_FILES = "qmldir"
# TEMPLATE = "lib"

# special case begin
qt_autogen_tools_initial_setup(childplugin)
file(COPY qmldir
    DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/../../imports/org/qtproject/AutoTestQmlPluginType/ChildPlugin"
)
# special case end
