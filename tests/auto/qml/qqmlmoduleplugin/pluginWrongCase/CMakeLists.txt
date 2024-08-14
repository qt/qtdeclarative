# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from pluginWrongCase.pro.

#####################################################################
## pluginWrongCase Generic Library:
#####################################################################

qt_internal_add_cmake_library(pluginWrongCase
    MODULE
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../imports/org/qtproject/WrongCase"
    SOURCES
        plugin.cpp
    LIBRARIES
        Qt::Core
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::Qml
        Qt::QmlPrivate
)

set_target_properties(pluginWrongCase PROPERTIES OUTPUT_NAME "Plugin")

#### Keys ignored in scope 1:.:.:pluginWrongCase.pro:<TRUE>:
# IMPORT_FILES = "qmldir"
# TEMPLATE = "lib"

qt_autogen_tools_initial_setup(pluginWrongCase)
# special case begin
file(COPY qmldir
    DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/../imports/org/qtproject/WrongCase"
)
# special case end
