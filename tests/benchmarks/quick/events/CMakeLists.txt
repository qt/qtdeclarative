# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from events.pro.

#####################################################################
## tst_qevents Binary:
#####################################################################

qt_internal_add_benchmark(tst_qevents
    SOURCES
        tst_events.cpp
    LIBRARIES
        Qt::CorePrivate
        Qt::Gui
        Qt::GuiPrivate
        Qt::Qml
        Qt::QmlPrivate
        Qt::Quick
        Qt::QuickPrivate
        Qt::Test
        Qt::QuickTestUtilsPrivate
)
