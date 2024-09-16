# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR BSD-3-Clause

qt_add_library(extra_module STATIC)
qt_add_qml_module(extra_module
    URI "ExtraModule"
    VERSION 1.0
    QML_FILES
        Extra.qml
    SOURCES
        extrathing.cpp extrathing.h
    RESOURCE_PREFIX /
)

add_subdirectory(ExtraModule)
