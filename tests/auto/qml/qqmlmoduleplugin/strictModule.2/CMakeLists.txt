# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from strictModule.2.pro.

#####################################################################
## strictModule.2 Generic Library:
#####################################################################

qt_internal_add_cmake_library(strictModule.2
    MODULE
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../imports/org/qtproject/StrictModule.2"
    SOURCES
        plugin.cpp
    LIBRARIES
        Qt::Core
        Qt::CorePrivate
        Qt::GuiPrivate
        Qt::Qml
        Qt::QmlPrivate
)

#### Keys ignored in scope 1:.:.:strictModule.2.pro:<TRUE>:
# IMPORT_FILES = "qmldir"
# TEMPLATE = "lib"

qt_autogen_tools_initial_setup(strictModule.2)
# special case begin
set_target_properties(strictModule.2 PROPERTIES OUTPUT_NAME "strictModule")
file(COPY qmldir
    DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/../imports/org/qtproject/StrictModule.2"
)
# special case end
