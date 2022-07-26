# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from qqmlcomponent.pro.

#####################################################################
## tst_qqmlcomponent Binary:
#####################################################################

qt_internal_add_benchmark(tst_qqmlcomponent_benchmark # special case avoid collision with auto test
    GUI
    SOURCES
        testtypes.cpp testtypes.h
        tst_qqmlcomponent.cpp
    DEFINES
        SRCDIR=\\\"${CMAKE_CURRENT_SOURCE_DIR}\\\"
    LIBRARIES
        Qt::Gui
        Qt::Qml
        Qt::Test
)

#### Keys ignored in scope 1:.:.:qqmlcomponent.pro:<TRUE>:
# TEMPLATE = "app"

## Scopes:
#####################################################################
