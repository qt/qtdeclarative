# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

# Generated from qml.pro.

add_subdirectory(binding)
add_subdirectory(compilation)
add_subdirectory(javascript)
add_subdirectory(holistic)
add_subdirectory(qqmlchangeset)
add_subdirectory(qqmlcomponent)
add_subdirectory(qqmlmetaproperty)
add_subdirectory(librarymetrics_performance)
add_subdirectory(script)
add_subdirectory(js)
add_subdirectory(creation)
add_subdirectory(qproperty)
if(TARGET Qt::OpenGL)
    add_subdirectory(qquickwindow)
endif()
if(TARGET Qt::OpenGL AND TARGET Qt::OpenGLWidgets)
    add_subdirectory(painting)
endif()
