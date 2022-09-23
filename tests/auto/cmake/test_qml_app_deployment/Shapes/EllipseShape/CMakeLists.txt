# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_add_qml_module(EllipseShape
    URI Shapes.EllipseShape
    VERSION 1.0
    SOURCES ellipseitem.cpp ellipseitem.h
    RESOURCE_PREFIX /
)

target_link_libraries(EllipseShape
    PRIVATE
        Qt::Quick
)
