# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt6_add_library(qmlqtimeexample2 STATIC)
qt_autogen_tools_initial_setup(qmlqtimeexample2)
qt6_add_qml_module(qmlqtimeexample2
    URI "TimeExample2"
    RESOURCE_PREFIX "/"
    SOURCES
        timemodel.cpp timemodel.h
    QML_FILES
        Clock.qml
    RESOURCES
        center.png
        clock.png
        hour.png
        minute.png
    DEPENDENCIES
        QtQml
)
qt_autogen_tools_initial_setup(qmlqtimeexample2plugin)
