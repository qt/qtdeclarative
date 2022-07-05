# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

qt_add_qml_module(EllipseShape
    URI Shapes.EllipseShape
    VERSION 1.0
    SOURCES ellipseitem.cpp ellipseitem.h
)

target_link_libraries(EllipseShape
    PRIVATE
        Qt::Quick
)
