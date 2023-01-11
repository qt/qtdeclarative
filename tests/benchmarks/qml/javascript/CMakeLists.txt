# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from javascript.pro.

#####################################################################
## tst_javascript Binary:
#####################################################################

qt_internal_add_benchmark(tst_javascript
    SOURCES
        testtypes.cpp testtypes.h
        tst_javascript.cpp
    DEFINES
        SRCDIR="${CMAKE_CURRENT_SOURCE_DIR}"
    LIBRARIES
        Qt::Gui
        Qt::Qml
        Qt::Test
)

#### Keys ignored in scope 1:.:.:javascript.pro:<TRUE>:
# TEMPLATE = "app"

## Scopes:
#####################################################################
