# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from holistic.pro.

#####################################################################
## tst_holistic Binary:
#####################################################################

qt_internal_add_benchmark(tst_holistic
    SOURCES
        testtypes.cpp testtypes.h
        tst_holistic.cpp
    DEFINES
        SRCDIR="${CMAKE_CURRENT_SOURCE_DIR}"
    LIBRARIES
        Qt::Gui
        Qt::Network
        Qt::Qml
        Qt::Test
)

#### Keys ignored in scope 1:.:.:holistic.pro:<TRUE>:
# TEMPLATE = "app"

## Scopes:
#####################################################################
