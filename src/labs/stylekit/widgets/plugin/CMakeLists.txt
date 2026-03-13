# Copyright (C) 2026 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

#####################################################################
## QStyleKitStylePlugin Plugin:
#####################################################################

qt_internal_add_plugin(QStyleKitStylePlugin
    OUTPUT_NAME qstylekitstyle
    PLUGIN_TYPE styles
    SOURCES
        main.cpp
    LIBRARIES
        Qt::Core
        Qt::Gui
        Qt::Widgets
        Qt::WidgetsPrivate
        Qt::LabsStyleKit
)
