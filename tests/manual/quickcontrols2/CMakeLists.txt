# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

# Generated from manual.pro.

add_subdirectory(buttons)
add_subdirectory(fonts)
add_subdirectory(gifs)
add_subdirectory(headerview)
add_subdirectory(qquickdialog)
add_subdirectory(screenshots)
add_subdirectory(styles)
add_subdirectory(styles-cover-flow)
add_subdirectory(testbench)
if(QT_FEATURE_systemtrayicon)
    add_subdirectory(systemtrayicon)
endif()
if(TARGET Qt::Widgets AND TARGET Qt::QuickWidgets)
    add_subdirectory(viewinqwidget)
endif()
