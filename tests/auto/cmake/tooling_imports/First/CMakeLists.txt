# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_add_qml_module(BackingFirst
    URI First
    VERSION 1.0
    SOURCES
        Neighbor.cpp
        Neighbor.h
    QML_FILES
        DoSomething.qml
    RESOURCE_PREFIX /
)
