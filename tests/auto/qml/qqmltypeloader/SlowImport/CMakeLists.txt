# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from Slow.pro.

#####################################################################
## Slow Generic Library:
#####################################################################

qt_internal_add_cmake_library(Slow MODULE # special case
    OUTPUT_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/../Slow"
    SOURCES
        plugin.cpp plugin.h
        slow.cpp slow.h
    INCLUDE_DIRECTORIES
        .
    PUBLIC_LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::Qml
        Qt::Quick
)

#### Keys ignored in scope 1:.:.:Slow.pro:<TRUE>:
# IMPORT_FILES = "qmldir"
# TEMPLATE = "lib"

# special case begin
qt_autogen_tools_initial_setup(Slow)
file (COPY qmldir
    DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/../Slow"
)
# special case end
