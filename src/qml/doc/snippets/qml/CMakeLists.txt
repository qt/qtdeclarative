# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

﻿qt_add_library(extra_module STATIC)
qt_add_qml_module(extra_module
    URI "ExtraModule"
    VERSION 1.0
    QML_FILES
        Extra.qml
    SOURCES
        extrathing.cpp extrathing.h
)

add_subdirectory(ExtraModule)
