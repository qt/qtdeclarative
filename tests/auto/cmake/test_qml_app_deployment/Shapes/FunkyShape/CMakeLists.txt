# Copyright (C) 2022 The Qt Company Ltd.
# SPDX-License-Identifier: BSD-3-Clause

qt_add_qml_module(FunkyShape
    URI Shapes.FunkyShape
    VERSION 1.0
    QML_FILES FunkyItemQml.qml
    SOURCES funkyitem.cpp funkyitem.h
    RESOURCE_PREFIX /
)
target_link_libraries(FunkyShape
    PRIVATE
        Qt::Quick
)
