# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: LicenseRef-Qt-Commercial OR GPL-3.0-only WITH Qt-GPL-exception-1.0

qt_add_qml_module(FunkyShape
    URI Shapes.FunkyShape
    VERSION 1.0
    QML_FILES FunkyItemQml.qml
    SOURCES funkyitem.cpp funkyitem.h
)
target_link_libraries(FunkyShape
    PRIVATE
        Qt::Quick
)
