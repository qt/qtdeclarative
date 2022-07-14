# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

qt6_add_library(qmlqtimeexample2 STATIC)
qt_autogen_tools_initial_setup(qmlqtimeexample2)
qt6_add_qml_module(qmlqtimeexample2
    VERSION 1.0
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
