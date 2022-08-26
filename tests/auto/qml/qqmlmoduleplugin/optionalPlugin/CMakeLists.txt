# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from optionalPlugin.pro.

#####################################################################
## optionalPlugin Generic Library:
#####################################################################

qt_internal_add_cmake_library(optionalPlugin
    MODULE
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../imports/QtQuick/Shapes"
    SOURCES
        optionalPlugin.cpp
    LIBRARIES
        Qt::Core
        Qt::Qml
)

#### Keys ignored in scope 1:.:.:optionalPlugin.pro:<TRUE>:
# IMPORT_FILES = "qmldir"
# TEMPLATE = "lib"

qt_autogen_tools_initial_setup(optionalPlugin)
# special case begin
file(COPY qmldir
    DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/../imports/QtQuick/Shapes"
)
# special case end
