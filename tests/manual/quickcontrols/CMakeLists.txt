# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from manual.pro.

add_subdirectory(buttons)
add_subdirectory(fonts)
if(LINUX AND QT_FEATURE_process)
    add_subdirectory(gifs)
endif()
add_subdirectory(headerview)
add_subdirectory(imagine/musicplayer)
add_subdirectory(menus)
add_subdirectory(qquickdialog)
add_subdirectory(screenshots)
add_subdirectory(sidepanel)
add_subdirectory(styles)
add_subdirectory(styles-cover-flow)
add_subdirectory(swipetoremove)
add_subdirectory(testbench)
if(QT_FEATURE_systemtrayicon)
    add_subdirectory(systemtrayicon)
endif()
if(TARGET Qt::Widgets AND TARGET Qt::QuickWidgets)
    add_subdirectory(viewinqwidget)
endif()
