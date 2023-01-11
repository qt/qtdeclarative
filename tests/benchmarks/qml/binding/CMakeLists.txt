# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from binding.pro.

#####################################################################
## tst_binding Binary:
#####################################################################

qt_internal_add_benchmark(tst_binding
    SOURCES
        testtypes.cpp testtypes.h
        tst_binding.cpp
    DEFINES
        SRCDIR="${CMAKE_CURRENT_SOURCE_DIR}"
    LIBRARIES
        Qt::Gui
        Qt::Qml
        Qt::Test
)

#### Keys ignored in scope 1:.:.:binding.pro:<TRUE>:
# TEMPLATE = "app"

## Scopes:
#####################################################################
