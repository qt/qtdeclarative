# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from foreign.pro.

#####################################################################
## foreign Generic Library:
#####################################################################

qt_internal_add_cmake_library(foreign
    STATIC
    SOURCES
        foreign.cpp foreign.h
        private/foreign_p.h
    PUBLIC_LIBRARIES
        Qt::Core
)
# special case begin
qt_enable_autogen_tool(foreign "moc" ON)
qt6_extract_metatypes(foreign)
# special case end

#### Keys ignored in scope 1:.:.:foreign.pro:<TRUE>:
# TEMPLATE = "lib"

## Scopes:
#####################################################################
